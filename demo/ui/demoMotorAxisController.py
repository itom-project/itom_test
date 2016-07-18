gui = ui("demoMotorAxisController.ui", ui.TYPEWINDOW)
#please notice, that some signals/slots are already connected via Designer!

c = gui.controller
motor = actuator("DummyMotor", 4)
c["actuator"] = motor
c["numAxis"] = 4 #the first 4 axes from the motor are considered in this case.
c["defaultAxisUnit"] = "UnitMum" #available: UnitNm (0), UnitMum (1), UnitMm (2), UnitCm (3), UnitM (4) or UnitDeg (5)
c["defaultAxisType"] = "TypeLinear" #available: TypeRotational (0), TypeLinear (1)
c["defaultRelativeStepSize"] = 0.010 #always in mm or deg
c["axisNames"] = ("x","y","z","alpha")
c["defaultDecimals"] = 2

#the fourth axis is a rotational axis:
c.call("setAxisType", 3, 0) #TypeRotational
c.call("setAxisUnit", 3, 5) #UnitDeg

gui.show()