# save the official lunch command to aosp_lunch() and source it
tmp_lunch=`mktemp`
sed '/ lunch()/,/^}/!d'  build/envsetup.sh | sed 's/function lunch/function aosp_lunch/' > ${tmp_lunch}
source ${tmp_lunch}
rm -f ${tmp_lunch}

# Override lunch function to filter lunch targets
function lunch
{
   aosp_lunch $*

   rm -rf Android.mk
   vendor/intel/utils/autopatch.sh

}

# Check that mixins are in sync with product configuration
if [ -f device/intel/mixins/mixin-update ]; then
    if ! device/intel/mixins/mixin-update --dry-run; then
        echo '+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++'
        echo '+ Product configuration and mixins are out of sync                      +'
        echo '+ PLEASE RE-RUN device/intel/mixins/mixin-update and commit the result! +'
        echo '+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++'
    fi
fi

