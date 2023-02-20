del *.bak
del *.ncb
del *.sdf

del SELasertrimming\Source\*.bak
del SELasertrimming\Source\*.ncb

del SELasertrimming\Resource\*.bak
del SELasertrimming\Resource\*.ncb
del SELasertrimming\Resource\*.aps

copy SELasertrimming\Debug\Release\*.htm SELasertrimming\Documentation\

attrib +S SELasertrimming\Debug\Debug\_Dummy.txt
del /Q SELasertrimming\Debug\Debug\*.*
attrib -S SELasertrimming\Debug\Debug\_Dummy.txt

attrib +S SELasertrimming\Debug\Release\_Dummy.txt
del /Q SELasertrimming\Debug\Release\*.*
attrib -S SELasertrimming\Debug\Release\_Dummy.txt

attrib +S SELasertrimming\Debug\Simulation_Debug\_Dummy.txt
del /Q SELasertrimming\Debug\Simulation_Debug\*.*
attrib -S SELasertrimming\Debug\Simulation_Debug\_Dummy.txt