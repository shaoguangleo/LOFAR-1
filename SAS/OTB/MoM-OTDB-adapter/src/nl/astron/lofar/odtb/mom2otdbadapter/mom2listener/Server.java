package nl.astron.lofar.odtb.mom2otdbadapter.mom2listener;

import java.io.IOException;
import java.net.ServerSocket;

import nl.astron.lofar.odtb.mom2otdbadapter.data.OTDBRepository;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class Server {
	private Log log = LogFactory.getLog(this.getClass());
	private OTDBRepository repository;
	
	public Server(OTDBRepository repository){
		this.repository =repository;
	}

	public void start() {
		ServerSocket server = null;
		try {
			log.info("Starting server on port 4444");
			server = new ServerSocket(4444);
		} catch (IOException e) {
			log.fatal("Could not listen on port 4444", e);
		}
		while (true) {
			ProcessConnection w;
			try {
				// server.accept returns a client connection
				w = new ProcessConnection(repository, server.accept());
				w.start();
			} catch (IOException e) {
				log.error("Accept failed: 4444", e);
			}
		}
	}

}
