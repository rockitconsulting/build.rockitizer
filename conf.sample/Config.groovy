/*
*	Copyright 2015 rockit.consulting GbR  (www.rockit.consulting)
*
*/
  
   
 
/* Uncomment if you have application */   
deployApplicationName='sample_app'

    
projectToBar = [
		['commons_folder':['MY_BARFILE_1_NOEXT','MY_BARFILE_2_NOEXT']],
  	    ['proj_folder_1':['MY_BARFILE_1_NOEXT']],
  	    ['proj_folder_2':['MY_BARFILE_2_NOEXT']],
]


brokerProjectDirs=[ // releative path to soure code of iib projects 
		     'conf.sample'
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
	    /* List of parent workspace with projects  comma separated. Relative paths supported. */   
		brokerProjectDirs=[
		    'C:\\temp',
		]
		
        barToExecutionGroup {
          MY_BARFILE_1_NOEXT.bar=['EX_GR_1']
          MY_BARFILE_2_NOEXT.bar=['EX_GR_2']
        }
    }
  
}



