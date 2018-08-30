[devicefiles]
device_file{{suffix}}.txt: "this file will be copied under product's directory"
device_dir{{suffix}}: "this directory will be copied under product's directory"

[extrafiles]
extra_file{{suffix}}.txt: "this file will be copied under product's extra_dir/template-complex-group directory"
extra_dir{{suffix}}: "this direcoty will be copied under product's extra_dir/template-complex-group directory"

[mapping]
extra_file{{suffix}}.txt: extra_file.txt
device_file{{suffix}}.txt: device_file.txt
extra_dir{{suffix}}: extra_dir
device_dir{{suffix}}: device_dir
