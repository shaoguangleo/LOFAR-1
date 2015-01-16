#!/usr/bin/env python

########################################################################
#                                                                      #
# Created by N.Vilchez (vilchez@astron.nl)                             #
# 14/11/2014                                                           #
#                                                                      #
########################################################################


########################################################################
# IMPORT general modules
########################################################################
import sys,os,glob,time
import getopt
import numpy as np

########################################################################
# Import local modules (classes)
########################################################################
from lofar.selfcal import class_obspar
from lofar.selfcal import class_mergesubbands
    

########################################################################
#							OVERVIEW
#
# 1) Inputs  
#
# 2) Warnings
#
# 3) Check parameters format and Initialize default values
#
# 3 bis) Parameter Overview check 
#
# 4) Observation Parameters extracted (i.e nof Subband's characteristics) 
#
# 5)Frequency  Concatenation
#
########################################################################


def main(initparameterlist):

	try:
	      opts, args = getopt.getopt(sys.argv[1:], "hoocn:",     ["help", "obsDir=", "outputDir=", "column2Merge=", "nofChannel2Merge=", "checkNames="])
      
	except getopt.GetoptError as err:
	      print ''
	      print "ERROR: Usage: mergeSB.py --obsDir= --outputDir= --column2Merge= [Optionnal: --nofChannel2Merge= --checkNames]"
	      print 'If nofChannel2Merge is not provided, by default it is merged in 1 channel per subband'
	      print 'If checkNames is not provided, by default checkNames is set to yes'
	      print ''
	      sys.exit(2)
	      
	         	
	for par1, par2 in opts:
		
		if par1 in ("--help"):
			  print ''
			  print "Usage: mergeSB.py --obsDir= --outputDir= --column2Merge= [Optionnal: --nofChannel2Merge= --checkNames]"
			  print 'If nofChannel2Merge is not provide, by default it is merged in 1 channel per subband'
			  print 'If checkNames is not provided, by default checkNames is set to yes'
			  print """
******************************************************************************
mergeSB Parameters:

*   --obsDir(string): This is the observation directory which contains data (or frequency merged data generated by mergeSB.py)
Data must contain the same number of subbands, have the same frequency range and the same pointing centre. Checks are done internally. The obsDir must contain ONLY data, no additional files. Due to this, if the outputDir is in the obsDir tree, the code will crash. Full path must be provided.
               
*  --outputDir(string): This is the Output directory where the selfcal process will be executed. If it doesn't exist, it will be created automatically.
Full path must be provided.

*  --column2Merge(string): This is the column to merge name. Usually CORRECTED_DATA is used (AWimager use only the CORRECTED_DATA column).

******************************************************************************
mergeSB OPTIONS:

*  --nofChannel2Merge(int, default: All subbands): This is the number of channel per subbands required after the Subbands concatenation.
By default, mergeSB concatenate subbands doing at the end 1 channel per subband.
 
* [OPTION NOT ACTIVATED AT THE MOMENT] --checkNames(string,default:yes): mergeSB checks names of MS (intermediate or final data products, see documentation). 
If checkNames set to no, a waring appears (in case MS names do not respect the nomenclature), but mergeSB continue without the exit.
******************************************************************************
"""	      
			  print ''
			  sys.exit(2)
        
        	
		elif par1 in ("--obsDir"):
			initparameterlist[0]=par2
		elif par1 in ("--outputDir"):
			initparameterlist[1]=par2		
		elif par1 in ("--column2Merge"):
			initparameterlist[2]=par2	
		elif par1 in ("--nofChannel2Merge"):
			initparameterlist[3]=par2	
		elif par1 in ("-- --checkNames"):
			initparameterlist[4]=par2												
		else:
        		print("Option {} Unknown".format(par1))
        		sys.exit(2)


        		
        # Check parameters		
 	if initparameterlist[0] == "none" or initparameterlist[1] == "none" or initparameterlist[2] == "none":
		print ""
		print "MISSING Parameters"	
		print ''
		print "Usage: mergeSB.py --obsDir= --outputDir= --column2Merge= [Optionnal: --nofChannel2Merge=  --checkNames]"
		print 'If nofChannel2Merge is not provide, by default it is merged in 1 channel per subband'
		print 'If checkNames is not provided, by default checkNames is set to yes'
		print ''
		print ''
		sys.exit(2)       	
 
 		
 	return initparameterlist 





    
########################################################################    
# Main Program
########################################################################    
    


if __name__=='__main__':
  
  
	####################################################################    
    # 1) Inputs
	####################################################################    
	
    initparameterlist=range(5)
    
    initparameterlist[0]	= "none"	# Observation Directory
    initparameterlist[1]	= "none"	# Output Directory
    initparameterlist[2]	= "none"	# column2Merge
    initparameterlist[3]	= "none"	# nofChannels 2 merge    
    initparameterlist[4]	= "none"	# checks file names  


    # Read and check parameters	
    initparameterlist 	= main(initparameterlist); 
  
    obsDir				= initparameterlist[0]
    outputDir			= initparameterlist[1]
    colum2Merge			= initparameterlist[2]
    nofChannel2Merge	= initparameterlist[3]
    checkNames			= initparameterlist[4]
    
    
    
	####################################################################    
    # 2) Warnings
	####################################################################    
    
    
    # WARNINGS on the obsDir 
    if obsDir[-1] != '/':
	obsDir = obsDir+'/'
	
    if os.path.isdir(obsDir) != True:
		print ""
		print "The observation directory do not exists ! Check it Please."
		print ""
		sys.exit(2)  
   
   
    # WARNINGS on the outputDir 
    if outputDir[-1] != '/':
		outputDir = outputDir+'/'
	
    if os.path.isdir(outputDir) != True:
		cmd="""mkdir %s"""%(outputDir)
		os.system(cmd)
		print ""
		print 'The output directory do not exists !\n%s has been created'%(outputDir)
		print ""
	
	
	####################################################################    
	# 3) Check parameters format and Initialize default values
	####################################################################    		
    
    if nofChannel2Merge != 'none':
		    try:
				  nofChannel2Merge = int(nofChannel2Merge)
				       
		    except:
				  print ""
				  print "nofChannel2Merge parameter must be an integer or not provided !"
				  print ""
				  sys.exit(2) 
				  
		    if nofChannel2Merge < 1 :
				  print ''
				  print 'The nofChannel2Merge parameter must be at least equal to 1:' 
				  print 'The provided value: %s channel per subband is wrong'%(nofChannel2Merge)
				  print ''
				  sys.exit(2)
					
		    multipleOfTwo	= 2**(np.arange(11))
		    if 	(nofChannel2Merge) in multipleOfTwo:
				  print ''
				  print 'Perfect: provided nofChannel2Merge is a power of 2: %s channel per subband'%(nofChannel2Merge)
				  print ''	
		    else:
				  print ''
				  print 'ERROR: provided nofChannel2Merge is not a power of 2: %s channel per subband'%(nofChannel2Merge)
				  print 'nofChannel2Merge must equal to one of these value:[1,2,4,8,16,32,64,128,256,512,1024]'	
				  print ''
				  sys.exit()								  
					
	

    if colum2Merge != 'none':
		    try:
				  colum2Merge = str(colum2Merge)     
		    except:
				  print ""
				  print "colum2Merge parameter must be an string and provided!"
				  print ""
				  sys.exit(2) 		

    if checkNames != 'none':
		    try:
				  checkNames = str(checkNames)     
		    except:
				  print ""
				  print "checkNames parameter must be a string or not provided !"
				  print ""
				  sys.exit(2) 
    else:
		checkNames = 'yes'
		
		
    if checkNames != 'no' and checkNames !='yes':
		print ""
		print "checkNames parameter must equal to yes or no !"
		print ""		   
   
 	####################################################################    
	# 3 bis) Parameter Overview check 
	####################################################################    

    print '############################################################'
    print ' 			PARAMETERS OVERVIEW                            '
    print '############################################################'
    print ''
    print 'obsDir = %s'%(obsDir)
    print ''
    print 'outputDir = %s'%(outputDir)	
    print ''
    print 'colum2Merge = %s'%(colum2Merge)	
    print ''
    print 'nofChannel2Merge = %s'%(nofChannel2Merge)	
    print ''
    print '[OPTION NOT ACTIVATED AT THE MOMENT] checkNames = %s'%(checkNames)
    print	 ''
    print '############################################################' 
   

   
	####################################################################    
	# 4) Observation Parameters extracted (i.e nof Subband's characteristics)
	####################################################################     

    
    print ''
    print '############################################################'
    print 'Start Observation directory global parameters determination'
    print '############################################################'
    print ''
  
    # Observation Directory Parameter determination
    obsPar_Obj	= class_obspar.observationParam(obsDir,checkNames)
    listFiles,obsType,IDs,SAPId,NbSB,StartSB,EndSB	= obsPar_Obj.obsParamFunc()    

    print '############################################################'
    print '					Extracted Subbands Infos'
    print '############################################################'	
    print '		IDs used:%s'%(IDs)
    print '		Number of subbands used:%s'%(NbSB)
    print '		Starting Subband:%s   Ending Subband:%s'%(StartSB,EndSB)
    print ''
    
    print '############################################################'
    print 'End Observation directory global parameters determination   '
    print '############################################################'
    print ''
    

 

	####################################################################    
	# 5) Frequency  Concatenation
	####################################################################  
    
	
    print '############################################################'
    print 'Start Frequency  Concatenation'
    print '############################################################'
    print ''
	
	
    mergeSubbands_Obj	= class_mergesubbands.mergeSubbands(obsDir,outputDir,colum2Merge,listFiles,IDs,obsType,SAPId,NbSB,StartSB,EndSB, nofChannel2Merge)
    mergeSubbands_Obj.mergeSubbandsFunc()

    print '############################################################'
    print 'End of time chunks concatenation'
    print '############################################################'

    
    #######################################################################################################################
