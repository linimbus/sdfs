dd if=/dev/zero of=test1.img bs=1KB  count=10000
dd if=/dev/zero of=test2.img bs=4KB  count=1000
dd if=/dev/zero of=test3.img bs=1MB  count=100
dd if=/dev/zero of=test4.img bs=4MB  count=10
dd if=/dev/zero of=test5.img bs=10MB count=1

dd if=test1.img of=/dev/null bs=1KB  count=10000
dd if=test2.img of=/dev/null bs=4KB  count=1000
dd if=test3.img of=/dev/null bs=1MB  count=100
dd if=test4.img of=/dev/null bs=4MB  count=10
dd if=test5.img of=/dev/null bs=10MB count=1
