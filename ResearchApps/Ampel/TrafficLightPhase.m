classdef TrafficLightPhase < Simulink.IntEnumType
  enumeration
    Off(0)
    OutOfOrder(-1)
    Red(1)
    Amber(2)
    RedAmber(3)
    Green(4)
  end
end 