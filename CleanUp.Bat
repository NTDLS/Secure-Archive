@Echo Off

RD Setup\Output /Y /DelRoot

Attrib Interface\*.* -h -r -a -s
RD Interface\Debug /Q /S
RD Interface\Release /Q /S
del Interface\*.suo
del Interface\*.user
del Interface\*.sdf
del Interface\Resources\*.aps

Attrib SelfExtractor\*.* -h -r -a -s
RD SelfExtractor\Debug /Q /S
RD SelfExtractor\Release /Q /S
del SelfExtractor\*.suo
del SelfExtractor\*.user
del SelfExtractor\*.sdf
del SelfExtractor\Resources\*.aps

Attrib ShellExtension\*.* -h -r -a -s
RD ShellExtension\Debug /Q /S
RD ShellExtension\Release /Q /S
del ShellExtension\*.suo
del ShellExtension\*.user
del ShellExtension\*.sdf
del ShellExtension\Resources\*.aps
