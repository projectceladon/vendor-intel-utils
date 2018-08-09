=== Overview

gms is used to configure GMS (Google Mobile Service) and GAS (Google AutoMotive Service)

=== Options

--- default
This option will disable GMS and GAS.


--- true
This option will enable GMS or GAS.

    --- parameters
        - car: true or false - enable GAS or enable GMS
        - minimal: true or false - enable or disable minimal GMS

    --- code dir
        - vendor/google/gms
        - vendor/google/gas
