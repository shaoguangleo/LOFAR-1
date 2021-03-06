"""RSP board register access functions

  def rspctl(tc, arg, applev=22)
  def rspctl_write_sleep()
  
  def i2bb(s)
  def i2bbbb(s)
  def swap2(s)
  def swap4(s)
  def calculate_next_sequence_value(in_word, seq='PRSG', width=12)
  def reorder(data, index)
  
  def write_mem(tc, msg, pid, regid, data, blpId=['blp0'], rspId=['rsp0'], width=2, offset=0, bc=0)
  def read_mem(tc, msg, pid, regid, nof, blpId=['blp0'], rspId=['rsp0'], sign='+', width=2, offset=0)
  
  def write_cr_syncoff(tc, msg, blpId=['blp0'], rspId=['rsp0'])
  def write_cr_syncon(tc, msg, blpId=['blp0'], rspId=['rsp0'])
  def write_cr_sync_delay(tc, msg, syncdelay=0, syncedge=0, blpId=['blp0'], rspId=['rsp0'])
  def read_cr_sync_delay(tc, msg, blpId=['blp0'], rspId=['rsp0'], applev=21)

  def write_serdes_rx_delay(tc, msg, clkDelay=0, rspId=['rsp0'], applev=21)
  def write_serdes_tx_delay(tc, msg, clkDelay=0, rspId=['rsp0'], applev=21)
  def read_serdes_rx_delay(tc, msg, rspId=['rsp0'], applev=21)
  def read_serdes_tx_delay(tc, msg, rspId=['rsp0'], applev=21)
  
  def write_diag_bypass(tc, msg, bypass, blpId=['blp0'], rspId=['rsp0'], applev=21)
  def read_diag_bypass(tc, msg, blpId=['blp0'], rspId=['rsp0'], applev=21)
  def write_diag_selftest(tc, msg, selftest, blpId=['blp0'], rspId=['rsp0'], applev=21)
  def read_diag_result_buffer(tc, msg, nof, width, blpId=['blp0'], rspId=['rsp0'])
  
  def write_rd_smbh_protocol_list(tc, msg, smbh, protocol_list,  polId=['x', 'y'], blpId=['blp0'], rspId=['rsp0'])
  def overwrite_rd_smbh_protocol_results(tc, msg, smbh, polId=['x', 'y'], blpId=['blp0'], rspId=['rsp0'])

  def write_rsu_altsync(tc, msg, rspId=['rsp0'])
  
  def overwrite_rsr(tc, msg, procid='all', value=255, rspId=['rsp0'])
  def read_rsr(tc, msg, procid='all', rspId=['rsp0'], applev=21)
  def write_rsr_timestamp(tc, msg, timestamp=0, timemode=1, fpgaId=['rsp'], rspId=['rsp0'], applev=21)
  def read_rsr_timestamp(tc, msg, fpgaId=['rsp', 'blp0'], rspId=['rsp0'], applev=21)
  def write_rsr_beam_mode(tc, msg, beam_mode=0, fpgaId=['rsp'], rspId=['rsp0'], applev=21)
  def read_rsr_beam_mode(tc, msg, fpgaId=['rsp'], rspId=['rsp0'], applev=21)
  def write_rsr_sdo_mode(tc, msg, sdo_mode=0, fpgaId=['rsp'], rspId=['rsp0'], applev=21)
  def read_rsr_sdo_mode(tc, msg, fpgaId=['rsp'], rspId=['rsp0'], applev=21)
  
  def write_rad_settings(tc, msg, settings, rspId=['rsp0'], applev=21)
  def read_rad_settings(tc, msg, rspId=['rsp0'], applev=21)
  def read_rad_latency(tc, msg, rspId=['rsp0'], applev=21)

  def write_cdo_ctrl(tc, msg, ctrl, rspId=['rsp0'], applev=21)
  def read_cdo_ctrl(tc, msg, rspId=['rsp0'], applev=21)
  def read_cdo_settings(tc, msg, rspId=['rsp0'], applev=21)
  def read_cdo_transport(tc, msg, rspId=['rsp0'], applev=21)
    
  def write_ss(tc, msg, ss_map, blpId=['blp0'], rspId=['rsp0'], bank=[0])
  def read_ss(tc, msg, nof, blpId=['blp0'], rspId=['rsp0'], bank=[0])
  def write_sdo_ss(tc, msg, ss_map, blpId=['blp0'], rspId=['rsp0'], bank=[0])
  def read_sdo_ss(tc, msg, nof, blpId=['blp0'], rspId=['rsp0'], bank=[0])
  
  def read_bf(tc, msg, nof, ppId=['xr'], blpId=['blp0'], rspId=['rsp0'], bank=[0])
"""

################################################################################
# System imports
import math
import time

# User imports
import cli
import mep
import smbus


################################################################################
# Constansts (from constants.tcl)

# From common(pkg).vhd
c_complex                   = 2       # factor 2 accounting for Re part and Im part of complex number
c_cpx                       = c_complex
c_phs                       = c_complex
c_pol                       = 2       # factor 2 accounting for X and Y polarization
c_pol_phs                   = c_pol * c_phs

c_long                      = 4

c_rcu_version               = 1  # ctrlreg[23:20] = "0001" in RCU i2cslave(rtl).vhd

c_ext_clk_freq              = 200000000
c_rcu_clk_freq              = c_ext_clk_freq

c_nof_bp                    = 1
c_nof_ap                    = 4
c_nof_blp_per_ap            = 1
c_nof_blp                   = c_nof_ap * c_nof_blp_per_ap

c_nof_rcu_per_ap            = c_pol                        # = 2, nof RCU per AP (and 1 BLP per AP)
c_nof_rcu                   = c_nof_ap*c_nof_rcu_per_ap    # = 8, nof RCU per RSP (and 1 BLP per AP)
c_nof_sp                    = c_nof_rcu;                   # = 8, nof signal paths (SP) per RSP (each RCU receives one SP)
  
c_nof_subbands              = 512
c_slice_size                = c_cpx * c_nof_subbands  # = 1024

c_nof_lanes                 = 4
c_nof_beamlets              = 248
c_nof_beamlets_per_lane     = c_nof_beamlets / c_nof_lanes
c_nof_antennas              = 96
c_nof_crosslets_per_antenna = 1
c_nof_crosslets             = c_nof_antennas * c_nof_crosslets_per_antenna
c_nof_crosslets_per_lane    = c_nof_crosslets / c_nof_lanes
c_nof_reflets               = c_nof_crosslets_per_antenna
c_nof_reflets_ap            = c_nof_crosslets_per_antenna * c_nof_ap     # = 4
c_nof_beamlets_ap           = c_nof_reflets_ap + c_nof_beamlets          # = 252
c_nof_let_types             = 2

c_rcu_dat_w                 = 12
c_rcu_dat_mask              = 2**c_rcu_dat_w - 1
c_blp_out_dat_w             = 18
c_ring_data_dat_w           = c_blp_out_dat_w+2        # = 20

c_diag_reg_wave_dat_w        = 18
c_diag_reg_wave_size_small_w = 11
c_diag_reg_res_word_w        =  4
c_diag_reg_res_size          =  c_nof_beamlets_ap*c_pol_phs

c_ss_map_size                = c_pol * c_nof_beamlets_ap  # = 504

c_bf_reg_coef_w              = 16
c_beam_data_dat_w            = 24

c_ring_dat_w                 = 14

c_tdsh_protocol_adr_w        = 11
c_tdsh_result_adr_w          = 10
c_rcuh_protocol_adr_w        =  9
c_rcuh_result_adr_w          =  9

c_rad_nof_rx_latency         =  1+c_nof_let_types*c_nof_lanes               # = 1 + 2*4     =  9  (RI, (Beamlets + Xrosslets) * 4)
c_rad_nof_rx_status          =  1+c_nof_let_types*c_nof_lanes+c_nof_lanes   # = 1 + 2*4 + 4 = 13  (RI, (Beamlets + Xrosslets) * 4, Subbands * 4)

c_ei_status_rsp_tvolt_offset =  0
c_ei_status_rsp_tvolt_size   =  9
c_ei_status_rsp_clk_offset   =  c_ei_status_rsp_tvolt_offset +                 c_ei_status_rsp_tvolt_size
c_ei_status_rsp_clk_size     =  1
c_ei_status_rsp_offset       =  0  # = c_ei_status_rsp_tvolt_offset
c_ei_status_rsp_size         = 12  # = c_ei_status_rsp_tvolt_size + c_ei_status_rsp_clk_size rounded to factor of 4
c_ei_status_eth_offset       =  c_ei_status_rsp_offset       +                 c_ei_status_rsp_size
c_ei_status_eth_size         = 12
c_ei_status_mep_offset       =  c_ei_status_eth_offset       +                 c_ei_status_eth_size
c_ei_status_mep_size         =  4
c_ei_status_diag_offset      =  c_ei_status_mep_offset       +                 c_ei_status_mep_size
c_ei_status_diag_size        = 24
c_ei_status_bs_offset        =  c_ei_status_diag_offset      +                 c_ei_status_diag_size
c_ei_status_bs_size          = 16  # For each AP
c_ei_status_rcuh_offset      =  c_ei_status_bs_offset        + c_nof_blp     * c_ei_status_bs_size
c_ei_status_rcuh_size        = 12  # For each AP
c_ei_status_rsu_offset       =  c_ei_status_rcuh_offset      + c_nof_blp     * c_ei_status_rcuh_size
c_ei_status_rsu_size         =  4
c_ei_status_ado_offset       =  c_ei_status_rsu_offset       +                 c_ei_status_rsu_size
c_ei_status_ado_size         =  8
c_ei_status_rad_offset       =  c_ei_status_ado_offset       + c_nof_blp     * c_ei_status_ado_size
c_ei_status_rad_size         =  4  # For each link
c_ei_status_rcuh_test_offset =  c_ei_status_rad_offset       + c_rad_nof_rx_status * c_ei_status_rad_size
c_ei_status_rcuh_test_size   =  4  # For each AP
c_ei_status_rsr_offset       =  0  # = c_ei_status_rsp_tvolt_offset
c_ei_status_rsr_size         =  c_ei_status_rcuh_test_offset + c_nof_blp     * c_ei_status_rcuh_test_size

c_ei_eth_err_noerr           = 0
c_ei_eth_err_preamblevalue   = 1
c_ei_eth_err_framedelimiter  = 2
c_ei_eth_err_preamblelength  = 3
c_ei_eth_err_headerlength    = 4
c_ei_eth_err_crc             = 5
c_ei_eth_err_oddnibblelength = 6
c_ei_eth_err_framelength     = 7

c_ei_mep_err_noerr           = 0
c_ei_mep_err_type            = 1
c_ei_mep_err_addr_blp        = 2
c_ei_mep_err_pid             = 3
c_ei_mep_err_regid           = 4
c_ei_mep_err_offset          = 5
c_ei_mep_err_size            = 6
c_ei_mep_err_ringcrc         = 7
c_ei_mep_err_timeout         = 8

# CDO = CEP Data Output (beamlets)
c_cdo_settings_size          = 30   # nof bytes in CDO settings register
c_cdo_settings_ctrl_offset   = 6    # byte index of the first byte of the ctrl field in CDO settings register
c_cdo_transport_size         = 28   # nof bytes in CDO transport header register (IPv4 + UDP = 20 + 8 = 28)

c_beam_mode_str              = ['1 x 16-bit', '2 x 8-bit', '4 x 4-bit']                # bm = 0, 1 or 2

# SDO = Subband Data Output
c_sdo_mode_str               = ['1 x 16-bit', '2 x 8-bit', '3 x 5-bit', '4 x 4-bit']   # sm = 0, 1, 2 or 3
c_ss_sel_zero                = 0x8000

c_sdo_nof_subbands           = 36                               # Number of SDO subbands per SP
c_sdo_nof_subbands_per_lane  = c_sdo_nof_subbands/c_nof_lanes

c_sdo_ss_nof_subbands_ap     = c_sdo_nof_subbands*c_pol         # = 72
c_sdo_ss_nof_subbands_rsp    = c_sdo_nof_subbands*c_nof_sp      # = 288
c_sdo_ss_nof_bands           = 316                              # = c_sdo_len_ap_frame_payload/c_pol

c_sdo_ss_map_size            = c_sdo_ss_nof_subbands_ap         # = 72


################################################################################
# Derived constants (from constants.tcl)

# - Calculate ADO scale factor based on word widths in ado(rtl).vhd and nof samples per sync interval for 200 MHz
dat_in_w    = c_rcu_dat_w
nof_acc_w   = math.ceil(math.log(c_rcu_clk_freq)/math.log(2))
result_w    = 32
acc_w       = dat_in_w + nof_acc_w
c_ado_scale = int(round(pow(2, acc_w - result_w)))

# RSR status
c_rsr_ok        = 0
c_rsr_error     = 1
c_rsr_undefined = 2

c_cp_bp              = 1
c_cp_statusRdy       = 0
c_cp_statusVersion0  = 1
c_cp_statusFpgaType  = 2
c_cp_statusImageType = 3
c_cp_statusTrigLo    = 4
c_cp_statusTrigHi    = 6
c_cp_statusVersion1  = 7
c_cp_version_w       = 2
c_cp_trig_w          = c_cp_statusTrigHi-c_cp_statusTrigLo+1
c_cp_trig_mask       = (1 << c_cp_trig_w) - 1
c_cp_trig_ButtonRst  = 0
c_cp_trig_TempRst    = 1
c_cp_trig_UserRst    = 2
c_cp_trig_WdRst      = 4

# From diag(pkg).vhd
c_diag_dev_ri           = 0
c_diag_dev_rcux         = 1
c_diag_dev_rcuy         = 2
c_diag_dev_lcu          = 3
c_diag_dev_cep          = 4
c_diag_dev_serdes       = 5

c_diag_mode_no_tst          =  0
c_diag_mode_loop_local      =  1
c_diag_mode_loop_line       =  2
c_diag_mode_loop_remote     =  3
c_diag_mode_tx              =  4
c_diag_mode_rx              =  5
c_diag_mode_tx_rx           =  6
# RI specific modes
c_diag_mode_bus             =  c_diag_mode_tx_rx
c_diag_mode_lane_all        =  7
c_diag_mode_lane_single     =  8
# Serdes specific loopback mode variants
c_diag_mode_loop_sys_diag   =  9
c_diag_mode_loop_par_diag   =  c_diag_mode_loop_local
c_diag_mode_loop_serial     = 10
c_diag_mode_loop_metal_line =  c_diag_mode_loop_line
c_diag_mode_loop_par_line   = 11

c_diag_duration_debug   = 0
c_diag_duration_quick   = 1
c_diag_duration_normal  = 2
c_diag_duration_extra   = 3

c_diag_res_ok           = 0
c_diag_res_none         = 1
c_diag_res_sync_timeout = 2
c_diag_res_data_timeout = 3
c_diag_res_word_err     = 4
c_diag_res_illegal      = 5

# RCU I2C bus
c_rcuh_i2c_addr_rcu     = 1
c_rcuh_i2c_addr_hba     = 2

# I2C handler time resolution
c_msec                  = int(round(200e6 * 1e-3))

# HBA control
#
#   In the client the registers are stored in order: SPEED, TBM, LED, DUMMY, VREF, STAT.
#   - TBM = measured bit time
#   - STAT = bit 0 contains the comparator state: 0 is high line input level, 1 for low line input level
#   Reading 2 bytes from the SPEED register yields SPEED and TBM. Similar reading 2 bytes from VREF yields
#   VREF and STAT. In fact note that reading 6 bytes from SPEED yields them all.
#
#   In the server the measured bit time is also stored after YDELAY, so via get word from c_hba_sreg_ydelay
#   one gets ydelay and the measured bit time.
c_hba_nof_servers     =  16       # HBA nof servers

c_hba_cmd_request     =   0       # HBA client REQUEST register
c_hba_cmd_response    =   1       # HBA client RESPONSE register
c_hba_cmd_led         =   2       # HBA client LED register
c_hba_cmd_vref        = 124       # HBA client VREF register (v10)
#c_hba_cmd_speed       = 127       # HBA client SPEED register (old)
c_hba_cmd_speed       = 125       # HBA client SPEED register (v10)
c_hba_cmd_version     = 126       # HBA client VERSION register
c_hba_reg_request_sz  =  38       # register size in octets
c_hba_reg_response_sz =   4
c_hba_reg_led_sz      =   1
c_hba_reg_vref_sz     =   1
c_hba_reg_speed_sz    =   1
c_hba_reg_version_sz  =   1

c_hba_f_set_byte      =   2       # HBA server function codes
c_hba_f_get_byte      =   3
c_hba_f_set_word      =   4
c_hba_f_get_word      =   5
c_hba_bc_server       =   0       # HBA server broadcast address
c_hba_sreg_xdelay     =   0       # HBA server xdelay register address
c_hba_sreg_ydelay     =   1       # HBA server ydelay register address
c_hba_sreg_version    = 254       # HBA server version register address
c_hba_sreg_address    = 255       # HBA server address register address

# - Modem time (modem speed = 40 = 10 kbps, modem prescaler = 1): 
#   . Broadcast request: 38 msec seems minimum for set_word broadcast 16 servers
#   . Unicast request  : 11 msec seems minimum for set_word or get word
# - For modem prescaler = 2 the uc_wait = 20 just works at modem speed = 40 = 5.5 kbps,
#   hence to be safe double the wait times.
# - Use hba_gap_wait as minimal wait after every I2C access to the client, before issueing a new I2C access
hba_prescaler  =  1
hba_prescaler  =  2
hba_bc_wait    = 40*hba_prescaler      # used with PROTOCOL_C_WAIT and WG_WAIT
hba_uc_wait    = 16*hba_prescaler      # used with PROTOCOL_C_WAIT and WG_WAIT
hba_gap_wait   =  2                    # used with PROTOCOL_C_WAIT and WG_WAIT
hba_bc_i2c     = 40       # used with WG_WAIT, I2C signalling time +30 is enough at 200 M
hba_uc_i2c     = 25       # used with WG_WAIT, I2C signalling time +20 is enough at 200 M
hba_reg_i2c    =  5       # used with WG_WAIT, I2C signalling time  +4 is enough at 50 kbps and 327 us comma per byte,
                          # so addr, cmd, 4 bytes = 6 bytes * 9 = 54 bits @ 50 kbps = 1.1 ms, 6 bytes * 327 us = 1.9 ms

c_hba_vref_default  = 0xEC  # Reference default mid level setting
c_hba_speed_default = 40
 
# TDS timing distribution clock board
c_tds_clksel_10MHz_sma    = 1
c_tds_clksel_10MHz_infini = 1 * (not c_tds_clksel_10MHz_sma)
c_tds_clksel_160MHz       = 0
c_tds_clksel_200MHz       = 1 * (not c_tds_clksel_160MHz)
c_tds_clksel_pps_sma      = 0
c_tds_clksel_pps_infini   = 1 * (not c_tds_clksel_pps_sma)

################################################################################
# Other constants (not from constants.tcl)
c_eth_block               = 1400

c_sens2v5                 =  2.5/192
c_sens3v3                 =  3.3/192
c_sens5v                  =  5.0/192
c_sens12v                 = 12.0/192

################################################################################
# Functions

def rspctl(tc, arg, applev=22):
  cmd = 'rspctl %s' % arg
  if tc != None:
    tc.appendLog(applev,cmd)     # 22: show command line
    res = cli.command(cmd)
    tc.appendLog(applev+1,res)   # 23: show command return
    return res
  else:
    return cli.command(cmd)
    

def rspctl_write_sleep():
  """ 
  The RSPDriver issues all --writeblock immediately to speed up the test case.
  Therefore sleep > 1 sec between rspctl --writeblock accesses to the same RSP
  to avoid that they get overwritten in the same pps interval. The RSPDriver
  can still only issue one --readblock per pps interval, because otherwise the
  read result gets lost. Hence for --readblock the test case is not speed up.
  """
  time.sleep(1.010)


def i2bb(s):
  """ Convert list of integers into list of byte-bytes
  Input:
  - s   = list of integers
  Return:
  - ret = list of two bytes, LSByte first per pair
  """
  ret = []
  for i in s:
    ret.extend([i%256, (i/256)%256])
  return ret

  
def i2bbbb(s):
  """ Convert list of integers into list of byte-byte-byte-bytes
  Input:
  - s   = list of integers
  Return:
  - ret = list of four bytes, LSByte first per four
  """
  ret = []
  for i in s:
    ret.extend([i%256, (i/256)%256, (i/(256*256))%256, (i/(256*256*256))%256])
  return ret
  

def swap2(i):
  """ Convert halfword size integer into byte-swapped integer
  Input:
  - i   = halfword size integer
  Return:
  - ret = byte-swapped integer
  """
  return ( (i%256)*256 + (i/256)%256 )


def swap4(i):
  """ Convert word size integer into byte-swapped integer
  Input:
  - i   = word size integer
  Return:
  - ret = byte-swapped integer
  """
  return ( (i%256)*256*256*256 + ((i/256)%256)*256*256 + ((i/256/256)%256)*256 + (i/256/256/256)%256 )
  
      
def calculate_next_sequence_value(in_word, seq='PRSG', width=12):
  """ Calculate next sequence value for PRSG or COUNTER
  
  Input:
  - in_word  = seed
  - seq      = PRSG   : use PRSG sequence as in RCU (width=12)
               others : use COUNTER sequence
  Return:
  - out_word = next sequence value after in_word
  """
  if seq=='PRSG':
    # Polynome
    w    = 2**width-1
    taps = [0, 3, 5, 11]
    
    # Feedback shift register
    p = 0
    for t in taps:
      b = 2**t
      b = 1 * (not (not (in_word & b)))
      p = p ^ b
    out_word = ((in_word << 1) & w) + 1 * (not p)
  else:
    w        = 2**width - 1
    out_word = (in_word + 1) & w
  return out_word

  
def reorder(data, index):
  """Reorder the data list according to the indices in the index list
  
  Input:
  - data  = Input data list
  - index = List with reorder indices
  Return:
            Reordered data list
  """
  nof    = len(data)
  redata = nof * [0]
  for i in range(nof):
    redata[index[i]] = data[i]
  return redata
  

def write_mem(tc, msg, pid, regid, data, blpId=['blp0'], rspId=['rsp0'], width=2, offset=0, bc=1):
  """Write data to memory register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - pid    = Process ID string
  - regid  = Register ID string
  - data   = List of words to write
  - blpId  = List of ['blp#']
  - rspId  = List of ['rsp#']
  - width  = Nof bytes per data word
  - offset = Offset in nof bytes
  - bc     = When !=0 use MEP broadcast
  Return: void
  """
  msg.packAddr(blpId, pid, regid)
  msg.packPayload(data,width)
  nof = width * len(data)       # nof octets
  for ri in rspId:
    i = 0
    n = c_eth_block
    while i < nof:
      if nof - i < n:
        n = nof - i
      hexData = msg.getPayload(i, n)
      if bc == 0:
        # Individual accesses to BLPs on an RSP board
        for bi in blpId:
          msg.packAddr([bi], pid, regid)
          rspctl(tc, '--writeblock=%s,%s,%d,%s' % (ri[3:], msg.hexAddr, offset+i, hexData))
          rspctl_write_sleep()
      else:
        # Make use of MEP broadcast to BLPs on an RSP board
        rspctl(tc, '--writeblock=%s,%s,%d,%s' % (ri[3:], msg.hexAddr, offset+i, hexData))
      i = i + n
  rspctl_write_sleep()


def read_mem(tc, msg, pid, regid, nof, blpId=['blp0'], rspId=['rsp0'], sign='+', width=2, offset=0):
  """Read data from memory register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - pid    = Process ID string
  - regid  = Register ID string
  - nof    = Nof bytes to read from the memory
  - blpId  = List of one ['blp#']
  - rspId  = List of one ['rsp#']
  - sign   = 'h' for hex string, '+' for unsigned word list, '-' for signed word list
  - width  = Nof bytes per data word
  - offset = Offset in nof bytes
  Return:
  -          list of read words
  """
  msg.packAddr(blpId, pid, regid)
  msg.clearPayload()
  i = 0
  n = c_eth_block
  while i < nof:
    if nof - i < n:
      n = nof - i
    msg.extractPayload(rspctl(tc, '--readblock=%s,%s,%d,%d' % (rspId[0][3:], msg.hexAddr, offset+i, n)), True)
    i = i + n
  if sign == 'h':
    return None  # use access via msg.hexPayload
  else:
    return msg.unpackPayload(width, sign)


def write_cr_syncoff(tc, msg, blpId=['blp0'], rspId=['rsp0']):
  """CR disable external sync
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - blpId  = List of 'rsp, blp#'
  - rspId  = List of 'rsp#'
  Return: void
  """
  for ri in rspId:
    msg.packAddr(blpId, 'cr', 'syncoff')
    msg.packPayload([1],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()
  

def write_cr_syncon(tc, msg, blpId=['blp0'], rspId=['rsp0']):
  """CR enable external sync
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - blpId  = List of 'rsp, blp#'
  - rspId  = List of 'rsp#'
  Return: void
  """
  for ri in rspId:
    msg.packAddr(blpId, 'cr', 'syncoff')
    msg.packPayload([0],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()


def write_cr_sync_delay(tc, msg, syncdelay=0, syncedge=0, blpId=['blp0'], rspId=['rsp0']):
  """CR write external sync input delay
  
  Input:
  - tc        = Testcase
  - msg       = MepMessage
  - syncdelay = 0 is reset input delay to hardware default, > 0 increment input delay one time
  - syncedge  = 0 is capture ext_sync on rising edge, != 0 is on falling edge
  - blpId     = List of 'rsp, blp#'
  - rspId     = List of 'rsp#'
  Return: void
  """
  bit0 = syncdelay > 0
  bit1 = syncedge  > 0
  syncdata = (bit1 << 1) + bit0
  for ri in rspId:
    msg.packAddr(blpId, 'cr', 'syncdelay')
    msg.packPayload([syncdata],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()


def read_cr_sync_delay(tc, msg, blpId=['blp0'], rspId=['rsp0'], applev=21):
  """CR read sync delay bit
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - blpId  = List of one 'rsp, blp#'
  - rspId  = List of one 'rsp#'
  - applev = Append logging level
  Return:
  - syncdata  = External sync data
                . bit 0 : '0' is default input delay, '1' is incremented delay
                . bit 1 : '0' capture external sync on rising edge, '1' on falling edge
 
    tc appendlog messages are reported.
  """
  syncdata = -1
  msg.packAddr(blpId, 'cr', 'syncdelay')
  readData = rspctl(tc, '--readblock=%s,%s,0,1' % (rspId[0][3:], msg.hexAddr))
  msg.extractPayload(readData)
  syncdata = msg.unpackPayload(1, '+')
  syncdata = syncdata[0]
  bit0 =  syncdata & 1
  bit1 = (syncdata & 2) >> 1
  if   bit0==0 and bit1==0:
    tc.appendLog(applev, '>>> RSP-%s, BLP-%-8s, read CR sync: default input delay,     default capture on rising edge' % (rspId, blpId))
  elif bit0==0 and bit1!=0:
    tc.appendLog(applev, '>>> RSP-%s, BLP-%-8s, read CR sync: default input delay,     capture on falling edge' % (rspId, blpId))
  elif bit0!=0 and bit1==0:
    tc.appendLog(applev, '>>> RSP-%s, BLP-%-8s, read CR sync: incremented input delay, default capture on rising edge' % (rspId, blpId))
  else:
    tc.appendLog(applev, '>>> RSP-%s, BLP-%-8s, read CR sync: incremented input delay, capture on falling edge' % (rspId, blpId))
  return syncdata
  

def write_serdes_rx_delay(tc, msg, clkDelay=0, rspId=['rsp0'], applev=21):
  """ SERDES write RX_CLK output delay
  
  Input:
  - tc       = Testcase
  - msg      = MepMessage
  - rspId    = List of 'rsp#'
  - clkDelay = Rx clock input delay
               . bit 0 : '0' is reset to default delay, '1' is increment delay
  Return: void
  """
  bit0 = clkDelay & 1
  for ri in rspId:
    msg.packAddr(['rsp'], 'serdes', 'rxdelay')
    msg.packPayload([bit0],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
    print 'rspctl --writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload)
  rspctl_write_sleep()
  
  
def write_serdes_tx_delay(tc, msg, clkDelay=0, rspId=['rsp0'], applev=21):
  """ SERDES write TX_CLK input delay
  
  Input:
  - tc       = Testcase
  - msg      = MepMessage
  - rspId    = List of 'rsp#'
  - clkDelay = Tx clock output delay
               . bit 0 : '0' is reset to default delay, '1' is increment delay
  Return: void
  """
  bit0 = clkDelay & 1
  for ri in rspId:
    msg.packAddr(['rsp'], 'serdes', 'txdelay')
    msg.packPayload([bit0],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()

  
def read_serdes_rx_delay(tc, msg, rspId=['rsp0'], applev=21):
  """ SERDES read RX_CLK input delay
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append log level
  Report:
    tc appendlog messages are reported.
  Return:
  - clkDelay  = Rx clock input delay
                 . bit 0 : '0' is default input delay, '1' is incremented delay
  """
  clkDelay = -1
  msg.packAddr(['rsp'], 'serdes', 'rxdelay')
  readData = rspctl(tc, '--readblock=%s,%s,0,1' % (rspId[0][3:], msg.hexAddr))
  print 'rspctl --readblock=%s,%s,0,1' % (rspId[0][3:], msg.hexAddr)
  msg.extractPayload(readData)
  clkDelay = msg.unpackPayload(1, '+')
  clkDelay = clkDelay[0]
  bit0 = clkDelay & 1
  if bit0==0:  tc.appendLog(applev, '>>> RSP-%s, read SERDES rx_clk : default input delay' % rspId)
  else:        tc.appendLog(applev, '>>> RSP-%s, read SERDES rx_clk : incremented input delay' % rspId)
  return clkDelay

  
def read_serdes_tx_delay(tc, msg, rspId=['rsp0'], applev=21):
  """ SERDES read TX_CLK output delay
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append log level
  Report:
    tc appendlog messages are reported.
  Return:
  - clkDelay  = Tx clock output delay
                 . bit 0 : '0' is default output delay, '1' is incremented delay
  """
  clkDelay = -1
  msg.packAddr(['rsp'], 'serdes', 'txdelay')
  readData = rspctl(tc, '--readblock=%s,%s,0,1' % (rspId[0][3:], msg.hexAddr))
  msg.extractPayload(readData)
  clkDelay = msg.unpackPayload(1, '+')
  clkDelay = clkDelay[0]
  bit0 = clkDelay & 1
  if bit0==0:  tc.appendLog(applev, '>>> RSP-%s, read SERDES tx_clk : default output delay' % rspId)
  else:        tc.appendLog(applev, '>>> RSP-%s, read SERDES tx_clk : incremented output delay' % rspId)
  return clkDelay


def write_diag_bypass(tc, msg, bypass, fpgaId=['blp0'], rspId=['rsp0'], applev=21):
  """Write DIAG bypass register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - bypass = Bypass enable word
  - fpgaId = List of ['rsp', 'blp#']
  - rspId  = List of ['rsp#']
  - applev = Append logging level
  Return: void
  """
  tc.appendLog(applev, '>>> RSP-%s, BLP-%s, write DIAG bypass:' % (rspId, fpgaId))
  tc.appendLog(applev, '      bit(0) : Bypass DC                         = %d' % ((bypass &  1    ) > 0))
  tc.appendLog(applev, '      bit(1) : Bypass PFS                        = %d' % ((bypass & (1<<1)) > 0))
  tc.appendLog(applev, '      bit(2) : Bypass PFT                        = %d' % ((bypass & (1<<2)) > 0))
  tc.appendLog(applev, '      bit(3) : Bypass BF                         = %d' % ((bypass & (1<<3)) > 0))
  tc.appendLog(applev, '      bit(4) : SI enable X                       = %d' % ((bypass & (1<<4)) > 0))
  tc.appendLog(applev, '      bit(5) : SI enable Y                       = %d' % ((bypass & (1<<5)) > 0))
  tc.appendLog(applev, '      bit(6) : DIAG result buffer use sync       = %d' % ((bypass & (1<<6)) > 0))
  tc.appendLog(applev, '      bit(7) : DIAG result buffer use resync     = %d' % ((bypass & (1<<7)) > 0))
  tc.appendLog(applev, '      bit(8) : PFT switching disable             = %d' % ((bypass & (1<<8)) > 0))
  tc.appendLog(applev, '   bit(10:9) : DIAG result buffer for AP BM bank = %d' % ((bypass & (3<<9)) > 0))
  tc.appendLog(applev, '     bit(11) : DIAG result buffer for BP         = %d' % ((bypass & (1<<11)) > 0))
  tc.appendLog(applev, '     bit(12) : Page swap on system sync          = %d' % ((bypass & (1<<12)) > 0))
  tc.appendLog(applev, '     bit(13) : RAD tx beamlet disable            = %d' % ((bypass & (1<<13)) > 0))
  tc.appendLog(applev, '     bit(14) : RAD tx crosslet disable           = %d' % ((bypass & (1<<14)) > 0))
  tc.appendLog(applev, '     bit(15) : RAD tx subband disable            = %d' % ((bypass & (1<<15)) > 0))
    
  for ri in rspId:
    msg.packAddr(fpgaId, 'diag', 'bypass')
    msg.packPayload([bypass],2)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()
  

def read_diag_bypass(tc, msg, fpgaId=['blp0'], rspId=['rsp0'], applev=21):
  """Read DIAG bypass register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - fpgaId = List of one ['rsp'] or ['blp#']
  - rspId  = List of one ['rsp#']
  - applev = Append logging level
  Return:
  - bypass = Bypass bits
  """
  msg.packAddr(fpgaId, 'diag', 'bypass')
  readData = rspctl(tc, '--readblock=%s,%s,0,2' % (rspId[0][3:], msg.hexAddr))
  msg.extractPayload(readData)
  bypass = msg.unpackPayload(2, '+')
  bypass = bypass[0]
  
  tc.appendLog(applev, '>>> RSP-%s, BLP-%s, read DIAG bypass:' % (rspId, fpgaId))
  tc.appendLog(applev, '      bit(0) : Bypass DC                         = %d' % ((bypass &  1    ) > 0))
  tc.appendLog(applev, '      bit(1) : Bypass PFS                        = %d' % ((bypass & (1<<1)) > 0))
  tc.appendLog(applev, '      bit(2) : Bypass PFT                        = %d' % ((bypass & (1<<2)) > 0))
  tc.appendLog(applev, '      bit(3) : Bypass BF                         = %d' % ((bypass & (1<<3)) > 0))
  tc.appendLog(applev, '      bit(4) : SI enable X                       = %d' % ((bypass & (1<<4)) > 0))
  tc.appendLog(applev, '      bit(5) : SI enable Y                       = %d' % ((bypass & (1<<5)) > 0))
  tc.appendLog(applev, '      bit(6) : DIAG result buffer use sync       = %d' % ((bypass & (1<<6)) > 0))
  tc.appendLog(applev, '      bit(7) : DIAG result buffer use resync     = %d' % ((bypass & (1<<7)) > 0))
  tc.appendLog(applev, '      bit(8) : PFT switching disable             = %d' % ((bypass & (1<<8)) > 0))
  tc.appendLog(applev, '   bit(10:9) : DIAG result buffer for AP BM bank = %d' % ((bypass & (3<<9)) > 0))
  tc.appendLog(applev, '     bit(11) : DIAG result buffer for BP         = %d' % ((bypass & (1<<11)) > 0))
  tc.appendLog(applev, '     bit(12) : Page swap on system sync          = %d' % ((bypass & (1<<12)) > 0))
  tc.appendLog(applev, '     bit(13) : RAD tx beamlet disable            = %d' % ((bypass & (1<<13)) > 0))
  tc.appendLog(applev, '     bit(14) : RAD tx crosslet disable           = %d' % ((bypass & (1<<14)) > 0))
  tc.appendLog(applev, '     bit(15) : RAD tx subband disable            = %d' % ((bypass & (1<<15)) > 0))
  return bypass


def write_diag_selftest(tc, msg, selftest, blpId=['rsp'], rspId=['rsp0'], applev=21):
  """Write DIAG selftest register
  
  Input:
  - tc       = Testcase
  - msg      = MepMessage
  - selftest = Selftest list: [interface, mode, duration, line]
  - blpId    = List of ['rsp', 'blp#']
  - rspId    = List of ['rsp#']
  - applev   = Append logging level
  Return: void
  """
  tc.appendLog(applev, '>>> RSP-%s write DIAG selftest:' % rspId)
  tc.appendLog(applev, '      interface = %d' % selftest[0])
  tc.appendLog(applev, '      mode      = %d' % selftest[1])
  tc.appendLog(applev, '      duration  = %d' % selftest[2])
  tc.appendLog(applev, '      line      = %d' % selftest[3])
    
  for ri in rspId:
    msg.packAddr(blpId, 'diag', 'selftest')
    msg.packPayload(selftest,1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()
    

def read_diag_result_buffer(tc, msg, nof, width, blpId=['blp0'], rspId=['rsp0']):
  """Read DIAG result buffer
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - nof    = Nof words to read from the result buffer
  - width  = Word width of the result buffer samples, 1, 2, or 4
  - blpId  = List of one 'blp#'
  - rspId  = List of one 'rsp#'
  Return:
  - Read result buffer words
  """
  return read_mem(tc, msg, 'diag', 'result', width*nof, blpId, rspId, '-', width)


def write_rsu_altsync(tc, msg, rspId=['rsp0']):
  """RSU apply altsync
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of 'rsp#'
  Return: void
  """
  for ri in rspId:
    msg.packAddr(['rsp'], 'rsu', 'sysctrl')
    msg.packPayload([1],1)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()
  

def write_rd_smbh_protocol_list(tc, msg, smbh, protocol_list,  polId=['x', 'y'], blpId=['blp0'], rspId=['rsp0']):
  """Write and readback protocol list to SMBus handler in RSP. The list will take effect at next sync.

  Input:
  - tc            = Testcase
  - msg           = MepMessage
  - smbh          = SMBus handler: 'tdsh' or 'rcuh'
  - protocol_list = Protocol list
  - polId         = Polarization ID: x, y or 'x y' for RCUH, ignored for TDSH
  - blpId         = BLP ID: 'blp#' for RCUH, destination defaults to 'rsp' for TDSH
  - rspId         = RSP ID: 'rsp#'
  Report:
    tc.appendlog messages are reported.
    tc.setResult is set
  Return: void
  """
  # - Write the protocol list to the SMBH
  smbus.write_protocol_list(tc, msg, smbh, protocol_list, polId, blpId, rspId)
  rspctl_write_sleep()

  # - Read back the protocol list to verify that this is possible
  for ri in rspId:
    if smbh == 'tdsh':
      rb_protocol_list = smbus.readback_protocol_list(tc, msg, smbh, len(protocol_list), None, None, [ri])
      if protocol_list == rb_protocol_list:
        tc.appendLog(21, '>>> RSP-%s, TDSH : The protocol list READBACK went OK' % ri)
      else:
        tc.appendLog(11, '>>> RSP-%s, TDSH : The protocol list READBACK went wrong:' % ri)
        tc.appendLog(11, 'Expected protocol list: %s' % protocol_list)
        tc.appendLog(11, 'Readback protocol list: %s' % rb_protocol_list)
        tc.setResult('FAILED')
    elif smbh == 'rcuh':
      for bi in blpId:
        for pi in polId:
	  rb_protocol_list = smbus.readback_protocol_list(tc, msg, smbh, len(protocol_list), pi, [bi], [ri])
          if protocol_list == rb_protocol_list:
            tc.appendLog(21, '>>> RSP-%s, BLP-%s, RCUH-%s: The protocol list READBACK went OK' % (ri, bi, pi))
          else:
            tc.appendLog(11, '>>> RSP-%s, BLP-%s, RCUH-%s: The protocol list READBACK went wrong:' % (ri, bi, pi))
            tc.appendLog(11, 'Expected protocol list: %s' % protocol_list)
            tc.appendLog(11, 'Readback protocol list: %s' % rb_protocol_list)
            tc.setResult('FAILED')


def overwrite_rd_smbh_protocol_results(tc, msg, smbh, polId=['x', 'y'], blpId=['blp0'], rspId=['rsp0']):
  """Overwrite and read protocol result of SMBus handler in RSP.

  Input:
  - tc            = Testcase
  - msg           = MepMessage
  - smbh          = SMBus handler: 'tdsh' or 'rcuh'
  - polId         = Polarization ID: x, y or 'x y' for RCUH, ignored for TDSH
  - blpId         = BLP ID: 'blp#' for RCUH, destination defaults to 'rsp' for TDSH
  - rspId         = RSP ID: 'rsp#'
  Report:
    tc appendlog messages are reported.
    tc setResult is set
  Return: void
  """
  if smbh == 'rcuh':
    nof_result_bytes = 2**c_rcuh_result_adr_w
  if smbh == 'tdsh':
    nof_result_bytes = 2**c_tdsh_result_adr_w
  
  # - Overwrite first entries of protocol result register to be sure that the results will be fresh
  wr_result = []
  for i in range(nof_result_bytes):
    wr_result.append(17)      # Just some number not equal to 0, 1, 255 and < 256

  # - Overwrite
  smbus.overwrite_results(tc, msg, smbh, wr_result, polId, blpId, rspId)
  rspctl_write_sleep()

  # - Readback to verify overwrite
  for ri in rspId:
    if smbh == 'tdsh':
      rd_result = smbus.read_results(tc, msg, smbh, nof_result_bytes, None, None, [ri])
      if wr_result == rd_result:
        tc.appendLog(21, '>>> RSP-%s, TDSH : The protocol results OVERWRITE and read went OK' % ri)
      else:
        tc.appendLog(11, '>>> RSP-%s, TDSH : The protocol results OVERWRITE and read went wrong:' % ri)
        tc.appendLog(11, 'Expected protocol result: %s' % wr_result)
        tc.appendLog(11, 'Readback protocol result: %s' % rd_result)
        tc.setResult('FAILED')
    elif smbh == 'rcuh':
      for bi in blpId:
        for pi in polId:
          rd_result = smbus.read_results(tc, msg, smbh, nof_result_bytes, pi, [bi], [ri])
          if wr_result == rd_result:
            tc.appendLog(21, '>>> RSP-%s, BLP-%s, RCUH-%s: The protocol results OVERWRITE and read went OK' % (ri, bi, pi))
          else:
            tc.appendLog(11, '>>> RSP-%s, BLP-%s, RCUH-%s: The protocol results OVERWRITE and read went wrong:' % (ri, bi, pi))
            tc.appendLog(11, 'Expected protocol result: %s' % wr_result)
            tc.appendLog(11, 'Readback protocol result: %s' % rd_result)
            tc.setResult('FAILED')


def overwrite_rsr(tc, msg, procid='all', value=255, rspId=['rsp0']):
  """Overwrite the selected process fields of the RSP status register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - procid = Process ID : 'rsp', 'eth', 'mep', 'diag', 'bs', 'rcuh', 'rsu', 'ado', 'rad', 'rcuh_test' or 'all'
  - value  = Byte value to use for overwrite
  - rspId  = List of 'rsp#' 
  Return: void
  """
  if   procid == 'rsp'       : offset = c_ei_status_rsp_offset;       size =                      c_ei_status_rsp_size
  elif procid == 'eth'       : offset = c_ei_status_eth_offset;       size =                      c_ei_status_eth_size
  elif procid == 'mep'       : offset = c_ei_status_mep_offset;       size =                      c_ei_status_mep_size
  elif procid == 'diag'      : offset = c_ei_status_diag_offset;      size =                      c_ei_status_diag_size
  elif procid == 'bs'        : offset = c_ei_status_bs_offset;        size = c_nof_blp           *c_ei_status_bs_size
  elif procid == 'rcuh'      : offset = c_ei_status_rcuh_offset;      size = c_nof_blp           *c_ei_status_rcuh_size
  elif procid == 'rsu'       : offset = c_ei_status_rsu_offset;       size =                      c_ei_status_rsu_size
  elif procid == 'ado'       : offset = c_ei_status_ado_offset;       size = c_nof_blp           *c_ei_status_ado_size
  elif procid == 'rad'       : offset = c_ei_status_rad_offset;       size = c_rad_nof_rx_status *c_ei_status_rad_size
  elif procid == 'rcuh_test' : offset = c_ei_status_rcuh_test_offset; size = c_nof_blp           *c_ei_status_rcuh_test_size
  else:                        offset = c_ei_status_rsr_offset;       size =                      c_ei_status_rsr_size
  
  status = []
  for i in range(size):
    status.append(value)
  write_mem(tc, msg, 'rsr', 'status', status, ['rsp'], rspId, 1, offset)


def read_rsr(tc, msg, procid='all', rspId=['rsp0'], applev=21):
  """Read the selected process fields from the RSP status register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - procid   = Process ID : 'rsp', 'eth', 'mep', 'diag', 'bs', 'rcuh', 'rsu', 'ado', 'rad', 'rcuh_test', or 'all'
  - rspId    = List of 'rsp#' 
  - applev   = Append log level
  Return:
  - ret      = List of read process status fields, list structure: [[first RSP], ..., [last RSP]],
               where the data structure per RSP depends on the procid.
  - retb     = Boolean telling wither the status is OK or ERROR
  """
  ret_rsp = []   # Declare return result

  # Read all RSR fields
  for ri in rspId:
    ret = []
    retb = []
    
    read_mem(tc, msg, 'rsr', 'status', c_ei_status_rsr_size, ['rsp'], [ri], 'h', 1, c_ei_status_rsr_offset)
    
    msg.setOffset(c_ei_status_rsr_offset)
    # RSP status
    rsp_volt_1v2         = msg.readUnsigned(1)
    rsp_volt_2v5         = msg.readUnsigned(1)
    rsp_volt_3v3         = msg.readUnsigned(1)
    rsp_pcb_temp         = msg.readSigned(1)
    rsp_bp_temp          = msg.readSigned(1)
    rsp_ap0_temp         = msg.readSigned(1)
    rsp_ap1_temp         = msg.readSigned(1)
    rsp_ap2_temp         = msg.readSigned(1)
    rsp_ap3_temp         = msg.readSigned(1)
    rsp_clk              = msg.readUnsigned(1)
    rsp_rsvd             = msg.readUnsigned(2)
    # ETH status
    eth_nof_frames       = msg.readUnsigned(4)
    eth_nof_errors       = msg.readUnsigned(4)
    eth_last_error       = msg.readUnsigned(1)
    eth_rsvd             = msg.readUnsigned(3)
    # MEP status
    mep_seq_nr           = msg.readUnsigned(2)
    mep_prev_error       = msg.readUnsigned(1)
    mep_rsvd             = msg.readUnsigned(1)
    # DIAG status
    diag_interface       = msg.readUnsigned(1)
    diag_mode            = msg.readUnsigned(1)
    diag_ri_errors        = {}
    diag_ri_errors['rsp'] = msg.readUnsigned(2)
    diag_rcux_errors     = msg.readUnsigned(2)
    diag_rcuy_errors     = msg.readUnsigned(2)
    diag_eth_errors        = {}
    diag_eth_errors['LCU'] = msg.readUnsigned(2)
    diag_eth_errors['CEP'] = msg.readUnsigned(2)
    diag_serdes_errors   = msg.readUnsigned(2)
    diag_ri_errors['0']  = msg.readUnsigned(2)
    diag_ri_errors['1']  = msg.readUnsigned(2)
    diag_ri_errors['2']  = msg.readUnsigned(2)
    diag_ri_errors['3']  = msg.readUnsigned(2)
    diag_rsvd            = msg.readUnsigned(2)
    # BS status
    bs_ext_cnt    = {}
    bs_sync_cnt   = {}
    bs_sample_cnt = {}
    bs_slice_cnt  = {}
    for bi in range(c_nof_blp):
      bs_ext_cnt[bi]     = msg.readUnsigned(4)
      bs_sync_cnt[bi]    = msg.readUnsigned(4)
      bs_sample_cnt[bi]  = msg.readUnsigned(4)
      bs_slice_cnt[bi]   = msg.readUnsigned(4)
    # RCU status
    rcu_status    = {}
    rcu_nof_ovr_x = {}
    rcu_nof_ovr_y = {}
    for bi in range(c_nof_blp):
      rcu_status[bi]     = msg.readUnsigned(4)
      rcu_nof_ovr_x[bi]  = msg.readUnsigned(4)
      rcu_nof_ovr_y[bi]  = msg.readUnsigned(4)
    # RSU status
    rsu_cp_status  = msg.readUnsigned(4)
    # ADO status
    ado_x = {}
    ado_y = {}
    for bi in range(c_nof_blp):
      ado_x[bi]          = msg.readSigned(4)
      ado_y[bi]          = msg.readSigned(4)
    # RAD BP status
    rad_ri               = msg.readUnsigned(4)
    rad_lane = {}
    for la in range(c_nof_lanes):
      rad_lane[la,'crosslets'] = msg.readUnsigned(4)
      rad_lane[la,'beamlets']  = msg.readUnsigned(4)
    for la in range(c_nof_lanes):
      rad_lane[la,'subbands'] = msg.readUnsigned(4)
    # RCU test status
    rcu_test_x = {}
    rcu_test_y = {}
    for bi in range(c_nof_blp):
      rcu_test_x[bi] = msg.readUnsigned(2)
      rcu_test_y[bi] = msg.readUnsigned(2)

    # Report RSR fields indicated by procid
    tc.appendLog(applev, '')
    tc.appendLog(applev, '>>> RSR read outs for RSP-%s.' % ri)
    tc.appendLog(applev, '')

    if procid == 'rsp' or procid == 'all':
      tc.appendLog(applev, 'RSP board status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  supply 1.2V = %6.3f V   (= %3d * %5.3f)' % (rsp_volt_1v2 * c_sens2v5, rsp_volt_1v2, c_sens2v5))
      tc.appendLog(applev, '  supply 2.5V = %6.3f V   (= %3d * %5.3f)' % (rsp_volt_2v5 * c_sens3v3, rsp_volt_2v5, c_sens3v3))
      tc.appendLog(applev, '  supply 3.3V = %6.3f V   (= %3d * %5.3f)' % (rsp_volt_3v3 * c_sens5v,  rsp_volt_3v3, c_sens5v))
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  temp PCB = %4d degrees C' % rsp_pcb_temp)
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  temp BP  = %4d degrees C' % rsp_bp_temp)
      tc.appendLog(applev, '  temp AP0 = %4d degrees C' % rsp_ap0_temp)
      tc.appendLog(applev, '  temp AP1 = %4d degrees C' % rsp_ap1_temp)
      tc.appendLog(applev, '  temp AP2 = %4d degrees C' % rsp_ap2_temp)
      tc.appendLog(applev, '  temp AP3 = %4d degrees C' % rsp_ap3_temp)
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  clk rate = %4d MHz' % rsp_clk)
      tc.appendLog(applev, '')
      ret.append([rsp_volt_1v2, rsp_volt_2v5, rsp_volt_3v3, rsp_pcb_temp, rsp_bp_temp, rsp_ap0_temp, rsp_ap1_temp, rsp_ap2_temp, rsp_ap3_temp, rsp_clk])

    if procid == 'eth' or procid == 'all':
      tc.appendLog(applev, 'Ethernet status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  Number of frames = %d' % eth_nof_frames)
      tc.appendLog(applev, '  Number of errors = %d' % eth_nof_errors)
      if   eth_last_error == c_ei_eth_err_noerr          : tc.appendLog(applev, '  Last error       = OK')
      elif eth_last_error == c_ei_eth_err_preamblevalue  : tc.appendLog(applev, '  Last error       = Preamble value error')
      elif eth_last_error == c_ei_eth_err_framedelimiter : tc.appendLog(applev, '  Last error       = Frame delimiter error')
      elif eth_last_error == c_ei_eth_err_preamblelength : tc.appendLog(applev, '  Last error       = Preamble length error')
      elif eth_last_error == c_ei_eth_err_headerlength   : tc.appendLog(applev, '  Last error       = Frame header lenght error')
      elif eth_last_error == c_ei_eth_err_crc            : tc.appendLog(applev, '  Last error       = CRC error')
      elif eth_last_error == c_ei_eth_err_oddnibblelength: tc.appendLog(applev, '  Last error       = Odd nof nibbles error')
      elif eth_last_error == c_ei_eth_err_framelength    : tc.appendLog(applev, '  Last error       = Frame size error')
      else:                                                tc.appendLog(applev, '  Last error       = Illegal error code: %d' % eth_last_error)
      tc.appendLog(applev, '')
      ret.append([eth_nof_frames, eth_nof_errors, eth_last_error])
      
    if procid == 'mep' or procid == 'all':
      tc.appendLog(applev, 'MEP status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  Sequence number           = %d' % mep_seq_nr)
      if   mep_prev_error == c_ei_mep_err_noerr   : tc.appendLog(applev, '  Error status previous msg = OK')
      elif mep_prev_error == c_ei_mep_err_type    : tc.appendLog(applev, '  Error status previous msg = Unknown message type')
      elif mep_prev_error == c_ei_mep_err_addr_blp: tc.appendLog(applev, '  Error status previous msg = Illegal BLP address')
      elif mep_prev_error == c_ei_mep_err_pid     : tc.appendLog(applev, '  Error status previous msg = Invalid PID')
      elif mep_prev_error == c_ei_mep_err_regid   : tc.appendLog(applev, '  Error status previous msg = Register does not exist')
      elif mep_prev_error == c_ei_mep_err_offset  : tc.appendLog(applev, '  Error status previous msg = Offset too large')
      elif mep_prev_error == c_ei_mep_err_size    : tc.appendLog(applev, '  Error status previous msg = Message too large')
      elif mep_prev_error == c_ei_mep_err_ringcrc : tc.appendLog(applev, '  Error status previous msg = Ring CRC error')
      elif mep_prev_error == c_ei_mep_err_timeout : tc.appendLog(applev, '  Error status previous msg = Timeout')
      else:                                         tc.appendLog(applev, '  Error status previous msg = Illegal error code: %d' % mep_prev_error)
      tc.appendLog(applev, '')
      ret.append([mep_seq_nr, mep_prev_error])
      
    if procid == 'diag' or procid == 'all':
      tc.appendLog(applev, 'DIAG status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  Interface                 = %d' % diag_interface)
      tc.appendLog(applev, '  Mode                      = %d' % diag_mode)
      # - BP RI
      if   diag_ri_errors['rsp'] == c_diag_res_ok:
        tc.appendLog(applev, '  Test result RI-BP         = OK')
      elif diag_ri_errors['rsp'] == c_diag_res_none:
        tc.appendLog(applev, '  Test result RI-BP         = Nothing happened')
      elif diag_ri_errors['rsp'] == c_diag_res_sync_timeout:
        tc.appendLog(applev, '  Test result RI-BP         = Sync timeout')
      elif diag_ri_errors['rsp'] == c_diag_res_data_timeout:
        tc.appendLog(applev, '  Test result RI-BP         = Data timeout')
      elif diag_ri_errors['rsp'] == c_diag_res_word_err:
        tc.appendLog(applev, '  Test result RI-BP         = Data errors occured')
      else:
        tc.appendLog(applev, '  Test result RI-BP         = Unknown status %d' % diag_ri_errors['rsp'])
        tc.appendLog(applev, '  Test result RCU-X         = %d' % diag_rcux_errors)
        tc.appendLog(applev, '  Test result RCU-Y         = %d' % diag_rcuy_errors)
      # - ETH
      eth_interfaces = ['LCU', 'CEP']
      for ei in eth_interfaces:
        if   diag_eth_errors[ei] == c_diag_res_ok:
          tc.appendLog(applev, '  Test result %s           = OK' % ei)
        elif diag_eth_errors[ei] == c_diag_res_none:
          tc.appendLog(applev, '  Test result %s           = Nothing happened' % ei)
        elif diag_eth_errors[ei] == c_diag_res_data_timeout:
          tc.appendLog(applev, '  Test result %s           = Data timeout (= frame lost)' % ei)
        elif diag_eth_errors[ei] == c_diag_res_word_err:
          tc.appendLog(applev, '  Test result %s           = Word errors occured' % ei)
        else:
          tc.appendLog(applev, '  Test result %s           = Unknown status %d' % (ei, diag_eth_errors[ei]))
      # - SERDES lanes
      lane_mask = 2**c_nof_lanes-1
      lane_ref  = 2**c_nof_lanes
      for i in range(c_nof_lanes):
        lane_errors = (diag_serdes_errors >> (i*c_nof_lanes)) & lane_mask
        if   lane_errors == c_diag_res_ok:
          tc.appendLog(applev, '  Test result SERDES lane %d = OK' % i)
        elif lane_errors == c_diag_res_none:
          tc.appendLog(applev, '  Test result SERDES lane %d = nothing happened' % i)
        elif lane_errors == c_diag_res_sync_timeout:
          tc.appendLog(applev, '  Test result SERDES lane %d = sync timeout' % i)
        elif lane_errors == c_diag_res_data_timeout:
          tc.appendLog(applev, '  Test result SERDES lane %d = data timeout' % i)
        elif lane_errors == c_diag_res_word_err:
          tc.appendLog(applev, '  Test result SERDES lane %d = word errors occured' % i)
        elif lane_errors == c_diag_res_illegal:
          tc.appendLog(applev, '  Test result SERDES lane %d = illegal status %d' % (i, lane_errors))
        else:
          tc.appendLog(applev, '  Test result SERDES lane %d = unknown status %d' % (i, lane_errors))
      # - APs RI
      for ai in ['0','1','2','3']:
        if   diag_ri_errors[ai] == c_diag_res_ok:
          tc.appendLog(applev, '  Test result RI-AP%s        = OK' % ai)
        elif diag_ri_errors[ai] == c_diag_res_none:
          tc.appendLog(applev, '  Test result RI-AP%s        = Nothing happened.' % ai)
        elif diag_ri_errors[ai] == c_diag_res_sync_timeout:
          tc.appendLog(applev, '  Test result RI-AP%s        = Sync timeout.' % ai)
        elif diag_ri_errors[ai] == c_diag_res_data_timeout:
          tc.appendLog(applev, '  Test result RI-AP%s        = Data timeout.' % ai)
        elif diag_ri_errors[ai] == c_diag_res_word_err:
          tc.appendLog(applev, '  Test result RI-AP%s        = Data errors occured.' % ai)
        else:
          tc.appendLog(applev, '  Test result RI-AP%s        = Unknown status %d.' % (ai, diag_ri_errors[ai]))
      tc.appendLog(applev, '')
      ret.append([diag_interface, diag_mode, diag_ri_errors['rsp'], diag_rcux_errors, diag_rcuy_errors, diag_eth_errors['LCU'], diag_eth_errors['CEP'], diag_serdes_errors, diag_ri_errors['0'], diag_ri_errors['1'], diag_ri_errors['2'], diag_ri_errors['3']])
      
    if procid == 'bs' or procid == 'all':
      tc.appendLog(applev, 'BS status:')
      tc.appendLog(applev, '')
      for bi in range(c_nof_blp):
        st  = 'BLP-%s: ' % bi
        st += 'Ext_cnt = %u, '    % bs_ext_cnt[bi]
        st += 'Sync_cnt = %u, '   % bs_sync_cnt[bi]
        st += 'Sample_cnt = %u, ' % bs_sample_cnt[bi]
        st += 'Slice_cnt = %u.'   % bs_slice_cnt[bi]
        tc.appendLog(applev, '  %s' % st)
        ret.append([bs_ext_cnt[bi], bs_sync_cnt[bi], bs_sample_cnt[bi], bs_slice_cnt[bi]])
      tc.appendLog(applev, '')
      
    if procid == 'rcuh' or procid == 'all':
      tc.appendLog(applev, 'RCU status:')
      tc.appendLog(applev, '')
      for bi in range(c_nof_blp):
        st  = 'BLP-%s: ' % bi
        st += 'status = %u, '    % rcu_status[bi]
        st += 'nof_ovr_x = %u, ' % rcu_nof_ovr_x[bi]
        st += 'nof_ovr_y = %u.'  % rcu_nof_ovr_y[bi]
        tc.appendLog(applev, '  %s' % st)
        ret.append([rcu_status[bi], rcu_nof_ovr_x[bi], rcu_nof_ovr_y[bi]])
      tc.appendLog(applev, '')
      
    if procid == 'rsu' or procid == 'all':
      tc.appendLog(applev, 'RSU status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '  CP status = %d' % rsu_cp_status)
      tc.appendLog(applev, '')
      if ((rsu_cp_status >> c_cp_statusRdy) & 0x1) == 1:
        tc.appendLog(applev, '    [%d] statusRdy       = 1 : CP is done' % c_cp_statusRdy)
      else:
        tc.appendLog(applev, '    [%d] statusRdy       = 0 : CP is in some intermediate state' % c_cp_statusRdy)
      if ((rsu_cp_status >> c_cp_statusFpgaType) & 0x1) == c_cp_bp:
        tc.appendLog(applev, '    [%d] statusFpgaType  = %d : Image was loaded via JTAG' % (c_cp_statusFpgaType, c_cp_bp))
      else:
        tc.appendLog(applev, '    [%d] statusFpgaType  = %d : Image was loaded from flash' % (c_cp_statusFpgaType, 1 * (not c_cp_bp)))
      if ((rsu_cp_status >> c_cp_statusImageType) & 0x1) == 0:
        tc.appendLog(applev, '    [%d] statusImageType = 0 : Factory image is running' % c_cp_statusImageType)
      else:
        tc.appendLog(applev, '    [%d] statusImageType = 1 : User image is running' % c_cp_statusImageType)
      if   ((rsu_cp_status >> c_cp_statusTrigLo) & c_cp_trig_mask) == c_cp_trig_ButtonRst:
        tc.appendLog(applev, '  [%d:%d] statusTrig      = %d : Reconfiguration due to button reset' % (c_cp_statusTrigHi, c_cp_statusTrigLo, c_cp_trig_ButtonRst))
      elif ((rsu_cp_status >> c_cp_statusTrigLo) & c_cp_trig_mask) == c_cp_trig_TempRst:
        tc.appendLog(applev, '  [%d:%d] statusTrig      = %d : Reconfiguration due to over temperature' % (c_cp_statusTrigHi, c_cp_statusTrigLo, c_cp_trig_TempRst))
      elif ((rsu_cp_status >> c_cp_statusTrigLo) & c_cp_trig_mask) == c_cp_trig_UserRst:
        tc.appendLog(applev, '  [%d:%d] statusTrig      = %d : Reconfiguration due to user reset' % (c_cp_statusTrigHi, c_cp_statusTrigLo, c_cp_trig_UserRst))
      elif ((rsu_cp_status >> c_cp_statusTrigLo) & c_cp_trig_mask) == c_cp_trig_WdRst:
        tc.appendLog(applev, '  [%d:%d] statusTrig      = %d : Reconfiguration due to watchdog reset' % (c_cp_statusTrigHi, c_cp_statusTrigLo, c_cp_trig_WdRst))
      else:
        tc.appendLog(applev, '  [%d:%d] statusTrig      = %d : Unknown reconfiguration trigger' % (c_cp_statusTrigHi, c_cp_statusTrigLo, (rsu_cp_status >> c_cp_statusTrigLo) & c_cp_trig_mask))
      cp_version = (((rsu_cp_status >> c_cp_statusVersion1) & 0x1) << 1) + ((rsu_cp_status >> c_cp_statusVersion0) & 0x1)
      tc.appendLog(applev, '  [%d,%d] statusVersion   = %d : CP version number' % (c_cp_statusVersion1, c_cp_statusVersion0, cp_version))
      tc.appendLog(applev, '')
      ret.append([rsu_cp_status])
      
    if procid == 'ado' or procid == 'all':
      tc.appendLog(applev, 'ADC offset:')
      tc.appendLog(applev, '')
      for bi in range(c_nof_blp):
        nof_samples_psync = bs_slice_cnt[bi] * c_slice_size
        st = 'BLP-%s, RCU-X ADC offset = ' % bi
        if nof_samples_psync != 0:
          st += '%11.7f lsb    ' % (1.0 * ado_x[bi] * c_ado_scale / nof_samples_psync)
        else:
          st += '-----------'
        st += '(%10d * %u / %11.0f)' % (ado_x[bi], c_ado_scale, nof_samples_psync)
        tc.appendLog(applev, '  %s' % st)

        st = 'BLP-%s, RCU-Y ADC offset = ' % bi
        if nof_samples_psync != 0:
          st += '%11.7f lsb    ' % (1.0 * ado_y[bi] * c_ado_scale / nof_samples_psync)
        else:
          st += '-----------'
        st += '(%10d * %u / %11.0f)' % (ado_y[bi], c_ado_scale, nof_samples_psync)
        tc.appendLog(applev, '  %s' % st)

        ret.append([ado_x[bi], ado_y[bi]])
      tc.appendLog(applev, '')
      
    if procid == 'rad' or procid == 'all':
      tc.appendLog(applev, 'RAD BP frame rx status:')
      tc.appendLog(applev, '')
      tc.appendLog(applev, '                     Align  Sync   CRC    Frame cnt')
      retb = c_rsr_undefined              # default undefined
      st = 'RI               : '
      cnt = rad_ri & ((1<<18)-1)
      if cnt==0:
        st += '-      -      -      '
      else:
        # when rad is busy there is always input from RI, from lane only in case of multiple RSP
        retb = c_rsr_ok
        st += '-      '                              # not applicable for RI
        if (rad_ri & (1<<19))!=0: st += 'OK     '
        else: st += 'Error  '; retb = c_rsr_error    # sync error(s)
        if (rad_ri & (1<<18))==0: st += 'OK     '
        else: st += 'Error  '; retb = c_rsr_error    # CRC error(s)
      st += '%u' % cnt
      tc.appendLog(applev, '  %s' % st)
      ret.append(rad_ri)
      for la in range(c_nof_lanes):
        for let in ['crosslets', 'beamlets', 'subbands']:
          st = 'Lane-%d, %-9s: ' % (la, let)
          cnt = rad_lane[la, let] & ((1<<18)-1)
          if cnt==0:
            st += '-      -      -      '
          else:
            # input from lane can keep retb as set by input from RI, or cause retb to indicate error
            if (rad_lane[la, let] & (1<<20))==0: st += 'OK     '
            else: st += 'Error  '; retb = c_rsr_error  # frame(s) discarded
            if (rad_lane[la, let] & (1<<19))!=0: st += 'OK     '
            else: st += 'Error  '; retb = c_rsr_error  # sync error(s)
            if (rad_lane[la, let] & (1<<18))==0: st += 'OK     '
            else: st += 'Error  '; retb = c_rsr_error  # CRC error(s)
          st += '%u' % cnt
          tc.appendLog(applev, '  %s' % st)
          ret.append(rad_lane[la, let])
      tc.appendLog(applev, '')
      ret = [retb, ret]
      
    if procid == 'rcuh_test' or procid == 'all':
      tc.appendLog(applev, 'RCU test status:')
      tc.appendLog(applev, '')
      for bi in range(c_nof_blp):
        st  = 'BLP-%s: ' % bi
        st += 'rcu_test_x = 0x%x, ' % rcu_test_x[bi]
        st += 'rcu_test_y = 0x%x'   % rcu_test_y[bi]
        tc.appendLog(applev, '  %s' % st)
        ret.append([rcu_test_x[bi], rcu_test_y[bi]])
      tc.appendLog(applev, '')
      
    ret_rsp.append(ret)
  return ret_rsp


def write_rsr_timestamp(tc, msg, timestamp=0, timemode=1, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR write timestamp and timestamp_mode

  Input:
  - tc        = Testcase
  - msg       = MepMessage
  - timestamp = Timestamp value
  - timemode  = Timestamp mode (see read_rsr_timestamp for the available timestamp modes)
  - fpgaId    = List of ['rsp', 'blp#']
  - rspId     = List of 'rsp#' 
  - applev    = Append log level
  Report:
    tc appendlog messages are reported.
  Return: void
  """ 
  if timemode==1:
      mode_str = 'reset to -1'
  elif timemode==2:
      mode_str = 'auto increment'
  else:
      mode_str = 'no change'
  tc.appendLog(applev, '>>> RSP-%s, FPGA-%s write RSR timestamp = %d, behaviour at sync is %s):' % (rspId, fpgaId, timestamp, mode_str))
  timedata=i2bbbb([timestamp])
  timedata.append(timemode)
  for ri in rspId:
      msg.packAddr(fpgaId, 'rsr', 'timestamp')
      msg.packPayload([timedata],1)
      rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()
  
  
def read_rsr_timestamp(tc, msg, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR Timestamp

  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - fpgaId = List of ['rsp', 'blp#']
  - rspId  = List of ['rsp#']
  - applev = Append log level
  Report:  
    tc appendlog messages are reported.
  Return:
  - read timestamp if all are equal else -1
  """
  fpgaTimestamps      = []
  fpgaTimestamp_modes = []
  for ri in rspId:
      for fi in fpgaId:
          read_mem(tc, msg, 'rsr', 'timestamp', 5, [fi], [ri], 'h', 1, 0)
          msg.setOffset(0)
          fpgaTimestamps.append(msg.readUnsigned(4))
          fpgaTimestamp_modes.append(msg.readUnsigned(1))
  index = 0
  for ri in rspId:
      for fi in fpgaId:
          tc.appendLog(applev, '>>> RSR timestamps:')
          tc.appendLog(applev, '    . RSP-%s, FPGA-%s : %d' % (ri, fi, fpgaTimestamps[index]))
          index += 1
  index = 0
  for ri in rspId:
      for fi in fpgaId:
          tc.appendLog(applev, '>>> RSR timestamp behaviour at sync:')
          if fpgaTimestamp_modes[index]==1:
              tc.appendLog(applev, '    . RSP-%s, FPGA-%s : reset to -1' % (ri, fi))
          elif fpgaTimestamp_modes[index]==2:
              tc.appendLog(applev, '    . RSP-%s, FPGA-%s : auto increment' % (ri, fi))
          else:
              tc.appendLog(applev, '    . RSP-%s, FPGA-%s : no change' % (ri, fi))
          index += 1
      
  ret = fpgaTimestamps[0]
  for ti in fpgaTimestamps:
      if ret != ti:
          ret = -1
  return ret

    
def write_rsr_beam_mode(tc, msg, beam_mode=0, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR write beam mode

  Input:
  - tc        = Testcase
  - msg       = MepMessage
  - beam_mode = Select beam mode 0, 1, or 2
  - fpgaId    = List of ['rsp', 'blp#']
  - rspId     = List of 'rsp#' 
  - applev    = Append log level
  Report:
    tc appendlog messages are reported.
  Return: void
  """ 
  tc.appendLog(applev, '>>> RSP-%s, FPGA-%s write RSR beam mode = %d : %s:' % (rspId, fpgaId, beam_mode, c_beam_mode_str[beam_mode]))
  offset = 1
  for ri in rspId:
      msg.packAddr(fpgaId, 'rsr', 'beammode')
      msg.packPayload([beam_mode],1)
      rspctl(tc, '--writeblock=%s,%s,%d,%s' % (ri[3:], msg.hexAddr, offset, msg.hexPayload))
  rspctl_write_sleep()


def read_rsr_beam_mode(tc, msg, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR read beam mode

  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - fpgaId = List of ['rsp', 'blp#']
  - rspId  = List of ['rsp#']
  - applev = Append log level
  Report:  
    tc appendlog messages are reported.
  Return:
  - read active beam_mode if all are equal else -1
  """
  fpga_beam_modes_max = []  # Maximum beam mode 0, 1, or 2 that is supported in the hardware
  fpga_beam_modes_act = []  # Selected beam mode 0, 1, or 2 that is currently active
  for ri in rspId:
      for fi in fpgaId:
          read_mem(tc, msg, 'rsr', 'beammode', 2, [fi], [ri], 'h', 1, 0)
          msg.setOffset(0)
          beam_max = msg.readUnsigned(1)
          beam_act = msg.readUnsigned(1)
          fpga_beam_modes_max.append(beam_max)
          fpga_beam_modes_act.append(beam_act)
          tc.appendLog(applev, '>>> RSR beam mode:')
          tc.appendLog(applev, '    . RSP-%s, FPGA-%s : max = %d (read only), active = %d' % (ri, fi, beam_max, beam_act))
      
  ret = fpga_beam_modes_act[0]
  for si in fpga_beam_modes_act:
      if ret != si:
          ret = -1
  return ret


def write_rsr_sdo_mode(tc, msg, sdo_mode=0, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR write subband data output (SDO) mode

  Input:
  - tc        = Testcase
  - msg       = MepMessage
  - sdo_mode  = Select SDO mode 0, 1, 2 or 3
  - fpgaId    = List of ['rsp', 'blp#']
  - rspId     = List of 'rsp#' 
  - applev    = Append log level
  Report:
    tc appendlog messages are reported.
  Return: void
  """ 
  tc.appendLog(applev, '>>> RSP-%s, FPGA-%s write RSR SDO mode = %d : %s:' % (rspId, fpgaId, sdo_mode, c_sdo_mode_str[sdo_mode]))
  offset = 1
  for ri in rspId:
      msg.packAddr(fpgaId, 'rsr', 'sdomode')
      msg.packPayload([sdo_mode],1)
      rspctl(tc, '--writeblock=%s,%s,%d,%s' % (ri[3:], msg.hexAddr, offset, msg.hexPayload))
  rspctl_write_sleep()


def read_rsr_sdo_mode(tc, msg, fpgaId=['rsp'], rspId=['rsp0'], applev=21):
  """RSR read SDO mode

  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - fpgaId = List of ['rsp', 'blp#']
  - rspId  = List of ['rsp#']
  - applev = Append log level
  Report:  
    tc appendlog messages are reported.
  Return:
  - read active sdo_mode if all are equal else -1
  """
  fpga_SDO_modes_max = []  # Maximum sdo mode 0, 1, 2 or 3 that is supported in the hardware
  fpga_SDO_modes_act = []  # Selected sdo mode 0, 1, 2 or 3 that is currently active
  for ri in rspId:
      for fi in fpgaId:
          read_mem(tc, msg, 'rsr', 'sdomode', 2, [fi], [ri], 'h', 1, 0)
          msg.setOffset(0)
          sdo_max = msg.readUnsigned(1)
          sdo_act = msg.readUnsigned(1)
          fpga_SDO_modes_max.append(sdo_max)
          fpga_SDO_modes_act.append(sdo_act)
          tc.appendLog(applev, '>>> RSR sdo mode:')
          tc.appendLog(applev, '    . RSP-%s, FPGA-%s : max = %d (read only), active = %d' % (ri, fi, sdo_max, sdo_act))
      
  ret = fpga_SDO_modes_act[0]
  for si in fpga_SDO_modes_act:
      if ret != si:
          ret = -1
  return ret


def write_rad_settings(tc, msg, settings, rspId=['rsp0'], applev=21):
  """RAD_BP write settings

  Input:
  - tc       = Testcase
  - msg      = MepMessage
  - settings = Lane settings for beamlets and crosslets
  - rspId    = List of 'rsp#' 
  - applev   = Append log level
  Report:
    tc appendlog messages are reported (see read_rad_settings for lane mode definition).
  Return: void
  """ 
  tc.appendLog(applev, '>>> RSP-%s write RAD settings (= 0x%X):' % (rspId, settings))
  # beamlet lane modes
  for i in range(c_nof_lanes-1,-1,-1):
    lane_mode = (settings >> (8*i)) & 0x3
    if   lane_mode==0: tc.appendLog(applev, '      lane(%d): beamlet  mode local' % i)
    elif lane_mode==1: tc.appendLog(applev, '      lane(%d): beamlet  mode disable' % i)
    elif lane_mode==2: tc.appendLog(applev, '      lane(%d): beamlet  mode combine' % i)
    else:              tc.appendLog(applev, '      lane(%d): beamlet  mode remote' % i)
  # crosslet lane modes
  for i in range(c_nof_lanes-1,-1,-1):
    lane_mode = (settings >> (8*i + 2)) & 0x3
    if   lane_mode==0: tc.appendLog(applev, '      lane(%d): crosslet mode local' % i)
    elif lane_mode==1: tc.appendLog(applev, '      lane(%d): crosslet mode disable' % i)
    elif lane_mode==2: tc.appendLog(applev, '      lane(%d): crosslet mode combine' % i)
    else:              tc.appendLog(applev, '      lane(%d): crosslet mode remote' % i)
  for ri in rspId:
    msg.packAddr(['rsp'], 'rad', 'settings')
    msg.packPayload([settings],4)
    rspctl(tc, '--writeblock=%s,%s,0,%s' % (ri[3:], msg.hexAddr, msg.hexPayload))
  rspctl_write_sleep()


def read_rad_settings(tc, msg, rspId=['rsp0'], applev=21):
  """RAD_BP read settings
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append log level
  Report:
    tc appendlog messages are reported.
  Return:
  - settings = Lane settings for beamlets and crosslets
      lane mode: one byte for each lane
        format: XXXXAABB
        where XX = don't care
              AA = xlet mode
              BB = blet mode
         mode 00 = ignore remote data (only local)  DEFAULT
         mode 01 = disable
         mode 10 = combine local and remote data
         mode 11 = ignore local data (only remote)
  """
  msg.packAddr(['rsp'], 'rad', 'settings')
  readData = rspctl(tc, '--readblock=%s,%s,0,4' % (rspId[0][3:], msg.hexAddr))
  msg.extractPayload(readData)
  settings = msg.unpackPayload(4, '+')
  settings = settings[0]
  tc.appendLog(applev, '>>> RSP-%s read RAD settings (= 0x%X):' % (rspId, settings))
  # beamlet lane modes
  for i in range(c_nof_lanes-1,-1,-1):
    lane_mode = (settings >> (8*i)) & 0x3
    if   lane_mode==0: tc.appendLog(applev, '      lane(%d): beamlet  mode local' % i)
    elif lane_mode==1: tc.appendLog(applev, '      lane(%d): beamlet  mode disable' % i)
    elif lane_mode==2: tc.appendLog(applev, '      lane(%d): beamlet  mode combine' % i)
    else:              tc.appendLog(applev, '      lane(%d): beamlet  mode remote' % i)
  # crosslet lane modes
  for i in range(c_nof_lanes-1,-1,-1):
    lane_mode = (settings >> (8*i + 2)) & 0x3
    if   lane_mode==0: tc.appendLog(applev, '      lane(%d): crosslet mode local' % i)
    elif lane_mode==1: tc.appendLog(applev, '      lane(%d): crosslet mode disable' % i)
    elif lane_mode==2: tc.appendLog(applev, '      lane(%d): crosslet mode combine' % i)
    else:              tc.appendLog(applev, '      lane(%d): crosslet mode remote' % i)
  return settings


def read_rad_latency(tc, msg, rspId=['rsp0'], applev=21):
  """RAD_BP read latency

  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of 'rsp#'
  - applev = Append log level
  Report:  
    tc appendlog messages are reported.
  Return:
  - latency = RI and lane latencies crosslets and beamlets
  """
  latency = []
  nof_rd = 2*c_rad_nof_rx_latency
  
  title_str = '>>>         '
  title_str += 'Ri'
  for i in range(c_nof_lanes-1,-1,-1):
    title_str += '    X%d' % i
  for i in range(c_nof_lanes-1,-1,-1):
    title_str += '    B%d' % i
  tc.appendLog(applev, title_str)

  for ri in rspId:
    lat = []
    read_mem(tc, msg, 'rad', 'latency', nof_rd, ['rsp'], [ri], 'h', 1, 0)
    
    msg.setOffset(0)
    lane = {}
    for i in range(c_nof_lanes):
      lane[i,'b'] = msg.readUnsigned(2)
      lane[i,'x'] = msg.readUnsigned(2)
    ring = msg.readUnsigned(2)

    lat_str = 'RSP-%s' % ri
    lat_str += '%6d' % ring
    lat.append(ring)
    for i in range(c_nof_lanes-1,-1,-1):
      lat_str += '%6d' % lane[i,'x']
      lat.append(lane[i,'x'])
    for i in range(c_nof_lanes-1,-1,-1):
      lat_str += '%6d' % lane[i,'b']
      lat.append(lane[i,'b'])
    tc.appendLog(applev, lat_str)
    latency.append(lat)
  return latency

  
def write_cdo_ctrl(tc, msg, ctrl, rspId=['rsp0'], applev=21):
  """Write control field in CDO settings register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - ctrl   = CDO control word [15:0]
  - rspId  = List of ['rsp#']
  - applev = Append logging level
  Return: void
  """
  tc.appendLog(applev, '>>> RSP-%s, write the CTRL field in CDO settings register:' % rspId)
  tc.appendLog(applev, '      bit(0)    : CDO enable            = %d' % ((ctrl &   1           ) >> 0))
  tc.appendLog(applev, '      bit(2:1)  : Lane select           = %d' % ((ctrl & ((2**2 -1)<<1)) >> 1))
  tc.appendLog(applev, '      bit(3)    : Fiber balance enable  = %d' % ((ctrl & ( 1       <<3)) >> 3))
  tc.appendLog(applev, '      bit(4)    : ARP enable            = %d' % ((ctrl & ( 1       <<4)) >> 4))
  tc.appendLog(applev, '      bit(15:5) : Not used              = %d' % ((ctrl & ((2**11-1)<<5)) >> 5))
  
  width = 2   # ctrl field is 2 bytes wide
  bc = 0      # access bp, so no BLP broadcast on RSP
  write_mem(tc, msg, 'cdo', 'settings', [ctrl], ['rsp'], rspId, width, c_cdo_settings_ctrl_offset, bc)

    
def read_cdo_ctrl(tc, msg, rspId=['rsp0'], applev=21):
  """Read control field in CDO settings register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append logging level
  Return: void
  """
  # Read entire CDO settings register
  read_mem(tc, msg, 'cdo', 'settings', c_cdo_settings_size, ['rsp'], rspId, 'h', 1, 0)
  # Get and report only the CTRL field
  width = 2   # ctrl field is 2 bytes wide
  msg.setOffset(c_cdo_settings_ctrl_offset)
  ctrl = msg.readUnsigned(width)
  tc.appendLog(applev, '>>> RSP-%s, read the CTRL field in CDO settings register:' % rspId)
  tc.appendLog(applev, '      bit(0)    : CDO enable            = %d' % ((ctrl &   1           ) >> 0))
  tc.appendLog(applev, '      bit(2:1)  : Lane select           = %d' % ((ctrl & ((2**2 -1)<<1)) >> 1))
  tc.appendLog(applev, '      bit(3)    : Fiber balance enable  = %d' % ((ctrl & ( 1       <<3)) >> 3))
  tc.appendLog(applev, '      bit(4)    : ARP enable            = %d' % ((ctrl & ( 1       <<4)) >> 4))
  tc.appendLog(applev, '      bit(15:5) : Not used              = %d' % ((ctrl & ((2**11-1)<<5)) >> 5))
    

def read_cdo_settings(tc, msg, rspId=['rsp0'], applev=21):
  """Read CDO settings register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append logging level
  Return: void
  """
  # Read entire CDO settings register
  read_mem(tc, msg, 'cdo', 'settings', c_cdo_settings_size, ['rsp'], rspId, 'h', 1, 0)
  msg.setOffset(0)
  reg_station_id       = msg.readUnsigned(2)
  reg_configuration_id = msg.readUnsigned(2)
  reg_ffi              = msg.readUnsigned(2)
  reg_ctrl             = msg.readUnsigned(2)
  reg_nof_blocks       = msg.readUnsigned(1)
  reg_nof_beamlets     = msg.readUnsigned(1)
  reg_dst_mac_lo       = msg.readUnsigned(4)
  reg_dst_mac_hi       = msg.readUnsigned(2)
  reg_dst_mac = []
  x = i2bb([reg_dst_mac_hi])
  reg_dst_mac.append(x[1])      # swap to compensate for i2bb
  reg_dst_mac.append(x[0])
  x = i2bbbb([reg_dst_mac_lo])
  reg_dst_mac.append(x[3])      # swap to compensate for i2bbbb
  reg_dst_mac.append(x[2])
  reg_dst_mac.append(x[1])
  reg_dst_mac.append(x[0])
  dmac = reg_dst_mac
  reg_src_mac_lo       = msg.readUnsigned(4)
  reg_src_mac_hi       = msg.readUnsigned(2)
  reg_src_mac = []
  x = i2bb([reg_src_mac_hi])
  reg_src_mac.append(x[1])      # swap to compensate for i2bb
  reg_src_mac.append(x[0])
  x = i2bbbb([reg_src_mac_lo])
  reg_src_mac.append(x[3])      # swap to compensate for i2bbbb
  reg_src_mac.append(x[2])
  reg_src_mac.append(x[1])
  reg_src_mac.append(x[0])
  smac = reg_src_mac
  reg_dst_ip_addr      = msg.readUnsigned(4)
  x = i2bbbb([reg_dst_ip_addr])
  dip = []
  dip.append(x[3])      # swap to compensate for i2bbbb
  dip.append(x[2])
  dip.append(x[1])
  dip.append(x[0])
  reg_src_ip_addr      = msg.readUnsigned(4)
  x = i2bbbb([reg_src_ip_addr])
  sip = []
  sip.append(x[3])      # swap to compensate for i2bbbb
  sip.append(x[2])
  sip.append(x[1])
  sip.append(x[0])
  tc.appendLog(applev, '>>> RSP-%s, CDO settings:' % rspId)
  tc.appendLog(applev, '    Station ID                         = %d' % reg_station_id)
  tc.appendLog(applev, '    Configuration ID                   = 0x%X' % reg_configuration_id)
  tc.appendLog(applev, '    FFI                                = 0x%X' % reg_ffi)
  tc.appendLog(applev, '    CTRL                               = 0x%X' % reg_ctrl)
  tc.appendLog(applev, '    CTRL[0]    : CDO enable            = %d' % ((reg_ctrl &   1           ) >> 0))
  tc.appendLog(applev, '    CTRL[2:1]  : Lane select           = %d' % ((reg_ctrl & ((2**2 -1)<<1)) >> 1))
  tc.appendLog(applev, '    CTRL[3]    : Fiber balance enable  = %d' % ((reg_ctrl & ( 1       <<3)) >> 3))
  tc.appendLog(applev, '    CTRL[4]    : ARP enable            = %d' % ((reg_ctrl & ( 1       <<4)) >> 4))
  tc.appendLog(applev, '    CTRL[15:5] : Not used              = %d' % ((reg_ctrl & ((2**11-1)<<5)) >> 5))
  tc.appendLog(applev, '    Nof blocks                         = %d' % reg_nof_blocks)
  tc.appendLog(applev, '    Nof beamlets                       = %d' % reg_nof_beamlets)
  tc.appendLog(applev, '    Destination MAC                    = %X:%X:%X:%X:%X:%X' % (dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]))
  tc.appendLog(applev, '    Source MAC                         = %X:%X:%X:%X:%X:%X' % (smac[0], smac[1], smac[2], smac[3], smac[4], smac[5]))
  tc.appendLog(applev, '    Destination IP address             = %d:%d:%d:%d' % (dip[0], dip[1], dip[2], dip[3]))
  tc.appendLog(applev, '    Source IP address                  = %d:%d:%d:%d' % (sip[0], sip[1], sip[2], sip[3]))

    
def read_cdo_transport(tc, msg, rspId=['rsp0'], applev=21):
  """Read the UDP/IP fields in CDO transport header register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - rspId  = List of one 'rsp#'
  - applev = Append logging level
  Return: void
  """
  # Define empty class to be used as record
  class ip:
    pass
  class udp:
    pass
  # Read entire CDO settings register
  read_mem(tc, msg, 'cdo', 'iphdr', c_cdo_transport_size, ['rsp'], rspId, 'h', 1, 0)
  # Get and report transport header
  msg.setOffset(0)
  # IPv4 header
  #
  #  0       3 4     7 8            15 16   18 19                        31 
  # |----------------------------------------------------------------------|
  # | Version |  HLEN |    Services    |      Total Length                 |
  # |----------------------------------------------------------------------|
  # |       Identification             | Flags |    Fragment Offset        |
  # |----------------------------------------------------------------------|
  # |     TTL         |    Protocol    |      Header Checksum              |
  # |----------------------------------------------------------------------|
  # |              Source IP Address                                       |
  # |----------------------------------------------------------------------|
  # |              Destination IP Address                                  |
  # |----------------------------------------------------------------------|
  # |                                                                      |
  # |              IP Payload                                              |
  # |                                                                      |
  # |------------------------------------------------------------ // ------|
  version_hlen       =       msg.readUnsigned(1)
  ip.version         = (version_hlen >> 4) & 0xF
  ip.header_length   =  version_hlen       & 0xF
  ip.services        =       msg.readUnsigned(1)
  ip.total_length    = swap2(msg.readUnsigned(2))        # swap for network big endian to little endian
  ip.identification  = swap2(msg.readUnsigned(2))
  flags_fragment     = swap2(msg.readUnsigned(2))
  ip.flags           = (flags_fragment >> 13) & 0x7
  ip.fragment_offset =  flags_fragment        & 0x1FFF
  ip.time_to_live    =       msg.readUnsigned(1)
  ip.protocol        =       msg.readUnsigned(1)
  ip.header_checksum = swap2(msg.readUnsigned(2))
  ip.src_ip_addr     = swap4(msg.readUnsigned(4))
  x = i2bbbb([ip.src_ip_addr])
  dip = []
  dip.append(x[3])      # swap to compensate for i2bbbb
  dip.append(x[2])
  dip.append(x[1])
  dip.append(x[0])
  ip.dst_ip_addr     = swap4(msg.readUnsigned(4))
  x = i2bbbb([ip.dst_ip_addr])
  sip = []
  sip.append(x[3])      # swap to compensate for i2bbbb
  sip.append(x[2])
  sip.append(x[1])
  sip.append(x[0])
  # UDP header
  #
  #  0                               15 16                               31 
  # |----------------------------------------------------------------------|
  # |      Source Port                 |      Destination Port             |
  # |----------------------------------------------------------------------|
  # |      Total Length                |      Checksum                     |
  # |----------------------------------------------------------------------|
  # |                                                                      |
  # |                      UDP Payload                                     |
  # |                                                                      |
  # |----------------------------------------------------------- // -------|
  udp.src_port     = swap2(msg.readUnsigned(2))        # swap for network big endian to little endian
  udp.dst_port     = swap2(msg.readUnsigned(2))
  udp.total_length = swap2(msg.readUnsigned(2))
  udp.checksum     = swap2(msg.readUnsigned(2))
  
  tc.appendLog(applev, '>>> RSP-%s, read the UDP/IP fields in CDO transport header register:' % rspId)
  tc.appendLog(applev, '      IPv4 : version         = %d'   % ip.version)
  tc.appendLog(applev, '      IPv4 : header_length   = %d'   % ip.header_length)
  tc.appendLog(applev, '      IPv4 : services        = 0x%X' % ip.services)
  tc.appendLog(applev, '      IPv4 : total_length    = %d'   % ip.total_length)
  tc.appendLog(applev, '      IPv4 : identification  = 0x%X' % ip.identification)
  tc.appendLog(applev, '      IPv4 : flags           = 0x%X' % ip.flags)
  tc.appendLog(applev, '      IPv4 : fragment_offset = %d'   % ip.fragment_offset)
  tc.appendLog(applev, '      IPv4 : time_to_live    = %d'   % ip.time_to_live)
  tc.appendLog(applev, '      IPv4 : protocol        = 0x%X' % ip.protocol)
  tc.appendLog(applev, '      IPv4 : header_checksum = 0x%X' % ip.header_checksum)
  tc.appendLog(applev, '      IPv4 : src_ip_addr     = %d:%d:%d:%d' % (dip[0], dip[1], dip[2], dip[3]))
  tc.appendLog(applev, '      IPv4 : dst_ip_addr     = %d:%d:%d:%d' % (sip[0], sip[1], sip[2], sip[3]))
  tc.appendLog(applev, '      UDP  : src_port        = 0x%X' % udp.src_port)
  tc.appendLog(applev, '      UDP  : dst_port        = 0x%X' % udp.dst_port)
  tc.appendLog(applev, '      UDP  : total_length    = %d'   % udp.total_length)
  tc.appendLog(applev, '      UDP  : checksum        = 0x%X' % udp.checksum)

          
def write_ss(tc, msg, ss_map, blpId=['blp0'], rspId=['rsp0'], bank=[0], pid='ss'):
  """Write subband to beamlet mapping to SS register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - ss_map = List of words for subband to beamlet mapping, length <= c_ss_map_size
  - blpId  = List of 'blp#'
  - rspId  = List of 'rsp#'
  - bank   = List of regId(s) for SS banks 0, 1, 2 and/or 3
  - pid    = SS instance name (peripheral ID)
  Return: void
  """
  regId = bank[0]
  write_mem(tc, msg, pid, 'settings%d' % regId, ss_map, blpId, rspId, 2)


def read_ss(tc, msg, nof=c_ss_map_size, blpId=['blp0'], rspId=['rsp0'], bank=[0], pid='ss'):
  """Read subband to beamlet mapping from SS register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - nof    = Nof words to read from the SS register <= c_ss_map_size
  - blpId  = List of one 'blp#'
  - rspId  = List of one 'rsp#'
  - bank   = List of one regId for SS bank 0, 1, 2 or 3
  - pid    = SS instance name (peripheral ID)
  Return:
  - Read SS register words
  """
  width = 2
  regId = bank[0]
  return read_mem(tc, msg, pid, 'settings%d' % regId, width*nof, blpId, rspId, '+', width)
  
  
def write_sdo_ss(tc, msg, ss_map, blpId=['blp0'], rspId=['rsp0'], bank=[0]):
  """Write subband data output mapping to SDO-SS register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - ss_map = List of words for SDO mapping, length <= c_sdo_ss_map_size = c_sdo_ss_nof_subbands_ap
  - blpId  = List of 'blp#'
  - rspId  = List of 'rsp#'
  - bank   = List of regId(s) for SDO SS banks 0, 1, 2 and/or 3
  Return: void
  """
  write_ss(tc, msg, ss_map, blpId, rspId, bank, 'sdo_ss')


def read_sdo_ss(tc, msg, nof=c_sdo_ss_map_size, blpId=['blp0'], rspId=['rsp0'], bank=[0]):
  """Read subband data output mapping from SDO SS register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - nof    = Nof words to read from the SDO SS register <= c_sdo_ss_map_size = c_sdo_ss_nof_subbands_ap
  - blpId  = List of one 'blp#'
  - rspId  = List of one 'rsp#'
  - bank   = List of one regId for SDO SS bank 0, 1, 2 or 3
  Return:
  - Read SDO SS register words
  """
  return read_ss(tc, msg, nof, blpId, rspId, bank, 'sdo_ss')
    
  
def read_bf(tc, msg, nof, ppId=['xr'], blpId=['blp0'], rspId=['rsp0'], bank=[0]):
  """Read coefficients from BF register
  
  Input:
  - tc     = Testcase
  - msg    = MepMessage
  - nof    = Nof words to read from the BF register
  - ppId   = List of one BF coefficient pol-phase identifier xr, xi, yr, or yi
  - blpId  = List of one 'blp#'
  - rspId  = List of one 'rsp#'
  - bank   = List of one register bank identifier for beam mode bank 0, 1, 2 or 3
  Return:
  - Read BF register words
  """
  width = 2
  return read_mem(tc, msg, 'bf', 'coef%s%d' % (ppId[0], bank[0]), width*nof, blpId, rspId, '-', width)
  
  
################################################################################
# Try some functions on the python command line
if __name__ == "__main__":
  import sys
  sys.argv[1:]        # list of input arguments
