This is a test of userfaultfd's security.

Some data (app.c) is going to be migrated from src.c to dest.c via UNIX domain socket, in the support of userfaultfd. The data is a function which returns an int 0 or 1. And there is a hacking program, trace_write.c, to monitor and tamper with outputs of src.c, in this case, app.c will be tampered with. To be more precisely, it is the if-condition been tampered with. If hacked, the function will return 0, otherwise 1 is returned.

If the data haven't been encrypted, it is certain to be tampered with. But if you encrypt it, nothing happens. So the experiments shows us the userfaultfd itself doesn't give any guarantee on migrated data. It just put the data into a specific place, regardless of its integrity.

My concern is to add a verification mechanism to userfaulfd. To ensure data security not just for qemu, but for any other programs which doesn't contain a encryption layer itself.
