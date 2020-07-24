# build.rockITizer
DevOps Framework for build, configuration, packaging, deployment, IaC for IBM Integration Bus products

## Tasks 

### Build, Config, Deploy
task checkConfig(group:'build', dependsOn:["configure${decoupled}", deltaBars])
task checkConfigDeploy(group:'build', dependsOn:[ "configure${decoupled}", deltaBars ,deployBars])
task buildConfigDeploy(group:'build', dependsOn:[ createBars, "configure${decoupled}", deltaBars, deployBars])
task packConfigDeploy(group:'build', dependsOn:[ packageBars, "configure${decoupled}", deltaBars, deployBars])
task packConfigDeployAssemble(group:'build', dependsOn:[ packageBars, "configure${decoupled}", deltaBars, deployBars, assemble])

### Infrastructure

task createEnvironment(group:'build', dependsOn:[ packageBars, checkConfig, createMqEnv, createExecutionGroups])
task deleteEnvironment(group:'build', dependsOn:[deleteMqEnv, deleteExecutionGroups])

