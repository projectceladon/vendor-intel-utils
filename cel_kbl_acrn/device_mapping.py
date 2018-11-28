#/usr/bin/env/python

# Mapping database between variant names, and the identifiers that appear in the
# build fingerprint. Please keep this up-to-date as new devices are added
# This script gets put in the target-files-package and used by
# ota_deployment_fixup.

# Versions:
#   0 - (brand, product, device, fish_name, base_variant)
#       PLEASE NOTE that v0 does NOT include the version key.
#   1 - (brand, product, device, lunch, fish_name, base_variant)
#       v1 is the first version that includes the version key.

dmap = {
        "__version__" : 1,
        "gr_mrb" : ("intel", "broxton_p", "bxtp_abl", "r0_bxtp_abl", "bxtp_abl", "gr_mrb"),
        "gr_mrb_b1" : ("intel", "broxton_p", "bxtp_abl", "r0_bxtp_abl", "bxtp_abl", "gr_mrb_b1")
}
