/*
*	Copyright 2015 rockit.consulting GbR  (www.rockit.consulting)
*
*/


#############################################################
#		Property of rockit.consulting
#
#       
#       
#       Broker Build Tool
#       
#       Main features:
#       0. Gradle based, easy to use, extendable, build patterns based   
#       1. externalized from broject 
#       2. build config kept by project
#       3. high performant
#       4. force broker archiv and configuration synchronization
#       5. force placeholders consistency
#       6. daimler wildcards build-in support <wildcardBars>
#	    7. simultaneous run of different builds
#		8. default parallelized run of tasks <Config.parallelExecution>
#       9. override source/project bars mode <-PoverrideSourceJars=true>
#		10. add/replace bar to execution group <-PdeployOverwrite=true>
#       11. application/flow createBar modes <Config.deployApplicationName>
#       
#           
#        
#
#
#############################################################

  





1. Gradle setup for Broker 

gradle.properties 
org.gradle.java.home=C:\\Services\\IBM\\IB9\\jre17
brokerToolkitDir=C:\\"Program Files (x86)"\\IBM\\IntegrationToolkit90


2. create your own build analoge to conf.sample project


3. Usage
gradlew buildConfigDeploy -PconfigPath=pathtoyourbuilddir 


gradlew createEnvironment -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew buildConfigDeploy -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2




ENV Plugin: 

gradlew createEnvironment -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew deleteEnvironment -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2


gradlew createMqEnv -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew deleteMqEnv -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew createExecutionGroups -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew deleteExecutionGroups -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew startExecutionGroups -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2
gradlew stopExecutionGroups -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=yd2



4. new Environment, i.e EG16 HowTo

4.1 Conf.groovy new Environment with ExecutionGroups:
    eg16 {
		brokerProjectDirs=[
		    'C:\\work\\projects\\wpps~weiterentwicklung_zmbfuv2\\wpps\\modules\\wpps_broker\\projects',
		    'C:\\work\\projects\\COMMON_BROKER~weiterentwicklung_zmbfuv2\\COMMON_BROKER\\projects'
		]
        barToExecutionGroup {
          WPPS_PROCESS.bar=['EG16_WPPS_PROCESS']
          WPPS_SYSTEM_IN.bar=['EG16_WPPS_SYSTEM_IN']
          WPPS_SYSTEM_OUT.bar=['EG16_WPPS_SYSTEM_OUT']
        }
    }

4.2 new environment eg16.properties to replace placeholders
To get own queues , use unique environment dependent Idx in queues. 
 


4.3 Commands
gradlew createEnvironment -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=eg16
gradlew buildConfigDeploy -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=eg16
gradlew buildConfigDeploy -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=dect  -PdeployOverwrite=false -Pdecoupled

5. Test framework (analog Section 4. -Pdecoupled mode)
gradlew createEnvironment -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=eg16 -Pdecoupled
gradlew buildConfigDeploy -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=eg16 -Pdecoupled
gradlew generateConnectors -PconfigPath=c:\work\projects\ib9rockitizer\proj\wppsconf -Penv=eg16 -Pdecoupled



99. enabling Gradle Wrapper setup behind firewall  

gradle\wrapper\gradle-wrapper.properties
distributionUrl=http\://d032a521.emea.corpds.test:8080/share/gradle-2.4-bin.zip


Troubleshooting:
1. MQ Security 

BIP1044I: Connecting to the queue manager...
BIP1046E: Unable to connect with the queue manager (The user 'rockit2lp' is not authorized to connect to queue manager 'IB9QMGR' (MQ reason code 2035 while trying to connect)).

Solution: Disable Security for WMQ:
>runmqsc QMGRNAME
>ALTER QMGR CHLAUTH(DISABLED)

http://www-01.ibm.com/support/docview.wss?uid=swg21577137

C:\rockit\work\projects\trunk\build.rockitizer
gradlew createEnvironment -PconfigPath=C:\rockit\work\rockit\projects\intern\sandbox\CommonsTestApp\conf -Penv=yefym -i
gradlew packConfigDeploy -PconfigPath=C:\rockit\work\rockit\projects\intern\sandbox\CommonsTestApp\conf -Penv=demo