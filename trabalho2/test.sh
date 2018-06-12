find imgs_test/ -type f | while read -r line ; do ./threads $line 50; ./multiprocessos $line 50; done;
