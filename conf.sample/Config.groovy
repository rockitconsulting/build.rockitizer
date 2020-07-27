/*
*	Copyright 2015 rockit.consulting GbR  (www.rockit.consulting)
*
*/
  
   
 
/* Uncomment if you have application */   
deployApplicationName='sample_app'

/*
deployApplicationName='RockitizerDemo SecondAppForMultibar'
deployLibraryName='sharedLib staticLib'  
*/     
    
projectToBar = [
		['commons_folder':['MY_BARFILE_1_NOEXT','MY_BARFILE_2_NOEXT']],
  	    ['proj_folder_1':['MY_BARFILE_1_NOEXT']],
  	    ['proj_folder_2':['MY_BARFILE_2_NOEXT']],
]



forceFlowExclude = []
    

testProjectDir='C:\\temp\\test\\'    
    

broker = 'Broker.broker'
mqcfg = 'MQMON.CFG'





environments {

    iib10 { //decoupled qmgr
      queueManager {
	    	 name='IB10NODE' 
	    	 host='localhost'
	    	 listenerPort='1414'
	    	 svrconn='SYSTEM.BKR.CONFIG'
	    }
	    /* List of parent workspace with projects  comma separated */   
		brokerProjectDirs=[
		    'C:\\temp',
		]
		
        barToExecutionGroup {
          MY_BARFILE_1_NOEXT.bar=['EX_GR_1']
          MY_BARFILE_2_NOEXT.bar=['EX_GR_2']
        }
    }
  
}



