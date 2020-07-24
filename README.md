# build.rockITizer
DevOps Framework for build, configuration, packaging, deployment, IaC for IBM Integration Bus products


## Usage

Gradle Tasks tab:
```
createMqEnv packConfigDeployAssemble
```

Project settings tab: 
```
-PconfigPath=C:\\rockit\\projects\\github\\test.rockitizer.demo\\RockitizerDemo\\conf -Penv=demo -PdeployOverwrite=false -PbuildDir=C:\\rockit\gradle_build -PprojVersion=1.3 --info
```


## Tasks 

### Build, Config, Deploy
- task _**checkConfig**_ checks configuration consistency: all overridable properties are known by framework (existing in flows.properties), all environment dependent placeholders replaced
- task _**checkConfigDeploy**_ is an Ops task which checks the consistency for the given environment, configures the bars via baroverride and deployes the bar files
- task _**buildConfigDeploy**_: long runnning createBar and after that checkConfig, configure and deploy tasks
- task _**packConfigDeploy**_: fast packageBar and after that checkConfig, configure and ceploy tasks
- task _**packConfigDeployAssemble**_: fast packageBar and after that checkConfig, configure, deploy and packageDistribution tasks

### Infrastructure

Both tasks require the broker project to be configured in order to extract the queue- and executionGroup names. Also MQMON.CFG with relevant QMGR connections need to be placed into **conf** folder

- task _**createEnvironment**_ executes packageBar and checkConfig. After that createMQEnv and createExecutionGroup
- task _**deleteEnvironment**_ executes packageBar and checkConfig. After that deleteMQEnv and deleteExecutionGroup


### Config.groovy
```
/*   for multibar use:   deployApplicationName='RockitizerDemo SecondAppForMultibar'  */   
deployApplicationName='RockitizerDemo'
  
    
projectToBar = [
	        ['*':['RockitizerDemo']],
]

forceFlowExclude = []
    
testProjectDir='C:\\temp\\test\\'    
    

//broker file can be added to environment
broker = 'demo.broker'
//MO71 aka MQMON Connection file for creation of MQ Infrastructure
mqcfg = 'MQMON.CFG'

environments {
    //list of environments
    demo {    
        //decoupled iib10 qmgr
          queueManager {
    	    	 name='QM1' 
    	    	 host='localhost'
    	    	 listenerPort='1414'
    	    	 svrconn='SYSTEM.BKR.CONFIG'
          }
    	 brokerProjectDirs=[
    			'C:\\rockit\\projects\\github\\test.rockitizer.demo\\'
    	]
    		
        barToExecutionGroup {
          RockitizerDemo.bar=['default']
        }
    }
    //end of demo environment ... the more to come below
}
```
