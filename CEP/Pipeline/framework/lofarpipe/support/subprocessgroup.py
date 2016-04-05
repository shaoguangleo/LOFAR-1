import subprocess
import select
import time
from lofarpipe.support.lofarexceptions import PipelineException

class SubProcess(object):
    STDOUT = 1
    STDERR = 2

    def __init__(self, logger, cmd, cwd):
        """
          Start a subprocess for `cmd' in working directory `cwd'.

          Output is sent to `logger'.
        """

        self.cmd       = cmd
        self.completed = False

        self.process = subprocess.Popen(
                       cmd,
                       cwd=cwd,
                       stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)
        self.pid     = self.process.pid
        self.exit_status = None

        self.output_buffers = { self.STDOUT: "",
                                self.STDERR: "" }

        def print_logger(line):
            print line

        self.output_loggers = { self.STDOUT: logger.debug if logger else print_logger,
                                self.STDERR: logger.warn  if logger else print_logger }
        self.logger         = logger.info if logger else print_logger

    	self.logger("Subprocess started: %s" % self.cmd)

    def done(self):
        if self.completed:
            return True

        if self.process.poll() is None:
            return False

        # Process is finished, read remaining data and exit code
        (stdout, stderr) = self.process.communicate()
        self.exit_status = self.process.returncode

        self._addoutput(self.STDOUT, stdout, flush=True)
        self._addoutput(self.STDERR, stderr, flush=True)

        self.completed = True

    	self.logger("Subprocess completed: %s" % self.cmd)

        return True

    def fds(self):
        return [self.process.stdout, self.process.stderr]

    def read(self, fds):
        for fd in fds:
            if fd == self.process.stdout:
                self._addoutput(self.STDOUT, fd.read(4096))
            if fd == self.process.stderr:
                self._addoutput(self.STDERR, fd.read(4096))

    def _addoutput(self, stdtype, output, flush=False):
        buf = self.output_buffers[stdtype] + output
        lines = buf.split("\n")
        remainder = lines.pop() if lines else ""

        for l in lines:
            self.output_loggers[stdtype](l)

        if flush:
            self.output_loggers[stdtype](remainder)
            remainder = ""
             
        self.output_buffers[stdtype] = remainder


class SubProcessGroup(object):
        """
        A wrapper class for the subprocess module: allows fire and forget
        insertion of commands with a an optional sync/ barrier/ return
        """
        def __init__(self, logger=None, 
                     usageStats = None,
                     # Default CEP2 is max 8 cpu used
                     max_concurrent_processes = 8,  
                     # poll each 10 seconds: we have a mix of short and long 
                     # running processes
                     polling_interval = 10):        
            self.process_group = []
            self.logger = logger
            self.usageStats = usageStats
            self.running_process_count = 0
            self.max_concurrent_processes = max_concurrent_processes

            # list of command vdw pairs not started because the maximum
            # number of processes was reached
            self.processes_waiting_for_execution = []
            self.polling_interval = polling_interval

        def _start_process(self, cmd, cwd):
            """
            Helper function collection all the coded needed to start a process
            """
            # About to start a process, increase the counter
            self.running_process_count += 1

            # Run subprocess
            process = SubProcess(self.logger, cmd, cwd)
            
            # save the process
            self.process_group.append(process)

            # add to resource monitor if available
            if self.usageStats:
                self.usageStats.addPID(process.pid)

        def run(self, cmd_in, unsave=False, cwd=None):
            """
            Add the cmd as a subprocess to the current group: The process is
            started!
            cmd can be suplied as a single string (white space seperated)
            or as a list of strings
            """

            if isinstance(cmd_in, str): 
                cmd = cmd_in.split()
            elif isinstance(cmd_in, list):
                cmd = cmd_in
            else:
                raise Exception("SubProcessGroup.run() expects a string or" +
                    "list[string] as arguments suplied: {0}".format(type(cmd)))

            # We need to be able to limit the maximum number of processes
            if self.running_process_count >= self.max_concurrent_processes: 
                # Save the command string and cwd
                self.processes_waiting_for_execution.append((cmd, cwd))
            else:
                self._start_process(cmd, cwd)


        def wait_for_finish(self):
            """
            Wait for all the processes started in the current group to end.
            Return the return status of a processes in an dict (None of no 
            processes failed 
            This is a Pipeline component: If an logger is supplied the 
            std out and error will be suplied to the logger
            """
            collected_exit_status = []

            while self.running_process_count or self.processes_waiting_for_execution:
                # collect all unfinished processes
                processes = [p for p in self.process_group if not p.completed]

                # collect fds we need to poll
                fds = []
                for process in processes:
                    fds.extend(process.fds())

                # poll for data
                rlist, _, _ = select.select(fds, [], [], self.polling_interval)

                # let processed read their data
                for process in processes:
                    process.read(rlist)

                # check all the running processes for completion
                for process in self.process_group:
                    if process.completed:
                        # process completed earlier
                        continue

                    if not process.done():
                        # process still running
                        continue

                    # We have a completed process
                    exit_status = process.exit_status

                    # get the exit status
                    if exit_status != 0:
                        collected_exit_status.append((process.cmd, exit_status))

                    # Now update the state of the internal state
                    self.running_process_count -= 1

                # if there are less then the allowed processes running and
                # we have waiting processes start another on
                while self.running_process_count < self.max_concurrent_processes and self.processes_waiting_for_execution: 
                    # Get the last process
                    cmd, cwd = self.processes_waiting_for_execution.pop()

                    # start it
                    self._start_process(cmd, cwd)

            # If none of the processes return with error status
            if len(collected_exit_status) == 0:
                collected_exit_status = None

            return collected_exit_status

