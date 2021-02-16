# build.rockITizer
DevOps Framework for build, configuration, packaging, deployment and MQ/EG infrastructure management for IBM Integration Bus product family.

## Youtube Video: <a href="https://youtu.be/SMTk9EoYofs" target="_blank">Boosting DevOps productivity with Build.RockITizer for IIB product family</a>
[<img src="http://i3.ytimg.com/vi/SMTk9EoYofs/hqdefault.jpg" width="50%">](https://youtu.be/SMTk9EoYofs)


## Configuration
The sample configuration is to find under ${build.rockitizer.home}/conf.sample folder:
- bars 
  - flows.properties - overridable properties autodiscovered from bar, the values can be configured over placeholders @key@
  - flows.properties.ignore - the overridable properties to ignore. All properties needs to be known by configuration system, otherwise the configuration integrity violated.
  - idep.properties - sample file for environment dependent properties following the naming convention:<env>.properties. The current active environment must be supplied over cmd. 
- broker files: <BROKER>.broker file for IIB connectivity. The environment specific file needs to be configured under respective environment in **Config.groovy**
- Config.groovy - **main configuration file** 
- MQMON.CFG - MO71 configuration file for the MQ Connectivity. All QMGRs needs to be configured in order to be able to create MQ Infrastructure from the flows.properties

---
**NOTE**

The best practice is to copy the conf.sample to conf in your project, as it holds the project specific configuration and documentation and will be added to assembly.

---



## Usage
Build can be triggered from any ide with gradle pluging or directly from command line. 

**Additonal PARAMS**

```bat
-PconfigPath - path to your project conf (same structure as conf.sample) folder, e.g. -PconfigPath=C:\\github\\RockitizerDemo\\conf
```

```bat
-Penv - your environment configured in Config.groovy, the correspondent <env>.porperties file with placeholder replacements must exist
```

```bat
-PbrokerFile=my.broker - support for agile deployment, overriding the environment configuration of Config.groovy 
```

```bat
-PdeployOverwrite=true|false  replace the whole applications during deployment (true) or just update them (false)
```

```bat
-PbuildDir=C:\\rockit\gradle_build providing working dir. Default specified in gradle.properties and is set to c:\\temp\\gradle_build
```

```bat
-PassemblyFileName=MyPackageName  Package name for assembly
```

```bat
-PassemblyFileVersion=1.3  Project version for assembly
```

```bat
-PdeployApplicationName=RockitizerDemo,SecondAppForMultibar  option for deploying of the app subset only 
```

```bat
-PenableMonitoring=true  enable (default)/disable flow monitoring on local/remote broker  
```

```bat
-PincrementalDeployment=true  enable (default)/disable incremental deployment of bars  
```

```bat
-PcleanFlowConfig=false  enable/disable (default) synchronisation of flows properties  
```

**Gradle Tasks tab (see Tasks below):**
```bat
createMqEnv packConfigDeployAssemble
```

**Project settings tab:**
```bat
-PconfigPath=C:\\github\\RockitizerDemo\\conf -Penv=demo -PdeployOverwrite=false -PbuildDir=C:\\rockit\gradle_build -PprojVersion=1.3 --info
```

## Tasks 

### Build, Config, Deploy
- task _**cleanBars**_ cleans execution groups/integrations server resources, as the default build mode has no preclean in order to support incremental deployment with multiple bars
- task _**checkConfig**_ checks configuration consistency: all overridable properties are known by framework (existing in flows.properties), all environment dependent placeholders replaced
- task _**checkConfigDeploy**_ is an Ops task which checks the consistency for the given environment, configures the bars via baroverride and deployes the bar files
- task _**buildConfigDeploy**_: long runnning createBar and after that checkConfig, configure and deploy tasks
- task _**packConfigDeploy**_: fast packageBar and after that checkConfig, configure and ceploy tasks
- task _**packConfigDeployAssemble**_: fast packageBar and after that checkConfig, configure, deploy and packageDistribution tasks
- task _**syncConfig**_: validate and clean flows configuration files

### Infrastructure

Both tasks require the broker project to be configured in order to extract the queue- and executionGroup names. Also MQMON.CFG with relevant QMGR connections need to be placed into **conf** folder

- task _**createEnvironment**_ executes packageBar and checkConfig. After that createMQEnv and createExecutionGroup
- task _**deleteEnvironment**_ executes packageBar and checkConfig. After that deleteMQEnv and deleteExecutionGroup


### Config.groovy
```groovy
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
