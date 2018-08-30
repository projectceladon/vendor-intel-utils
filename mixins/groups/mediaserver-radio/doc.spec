=== Overview

In order to support audio voice use cases, CoE Audio HAL SW architecture obliges
us to add the "radio" group permission to the media server service.
This group is needed in order to:
    open a MMGR (modem manager) socket
    open a gsmtty in order to send AT commands to the modem


=== Options

--- true
Add the "radio" group permission to the media server service.


--- default
when not explicitly selected in mixin spec file, the default option will be used.
It's empty dir.


