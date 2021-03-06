================================================================================
                    SmartFusion2 CorePWM edges control example 
================================================================================
This example demonstrates how to control the positive and negative edges of 
individual PWM outputs. This example generates the PWM waveforms described
in Figure 1-3 of the CorePWM DirectCore handbook.

--------------------------------------------------------------------------------
                            How to use this example
--------------------------------------------------------------------------------

This sample project is targeted at a Cortex-M3 design running on the SmartFusion2
Eval Kit (090) board. The example project is built for a design using a 
SmartFusion2 MSS APB clock frequency of 25MHz. Trying to execute this example
project on a different design will result in incorrect time being used by CorePWM.
The design should have an instance of CorePWM version 4.3.101 located at 
address 0x50002000.

 Make sure to set up following hardware configuration for running the current
 example project:
    - One hardware instance of CorePWM should be created with 4 PWM Channel.
    - PWM outputs 1-4 should be connected to board I/O pins. which can be
      monitored using logic analyzer.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                          Silicon revision dependencies
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
This example is built to execute on an M2S090T die. You will need to overwrite
this example project's "drivers_config/sys_config" and "CMSIS" folders with the
one generated by Libero for your hardware design if using a newer silicon revision.
The "drivers_config/sys_config" folder contains information about your hardware
design. This information is used by the CMSIS to initialize clock frequencies
global variables which are used by the SmartFusion2 drivers to derive baud
rates. The CMSIS boot code may also complete the device's clock configuration
depending on silicon version. The "CMSIS" and "drivers_config/sys_config" for
your design can be found in the "firmware" folder of your Libero design.
