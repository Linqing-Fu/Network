do as follow:

$ sudo python tcp_topo.py
$ xterm h1 h2 h3

in h2, h3:
$ ./tcp_stack server 10001

in h1:
$ ./tcp_stack client war_and_peace.txt 10001
