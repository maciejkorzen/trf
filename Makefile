DESTDIR=
prefix=/usr/local

all:
	@case "`uname -s`" in \
		Linux) \
			make linux \
			;; \
		FreeBSD) \
			make freebsd \
			;; \
		*) \
			echo "Nie wiem jakiego systemu używasz." ; \
			echo "Możesz użyć 'make linux' lub 'make freebsd'." \
			;; \
	esac

linux:
	gcc -DLINUX -Wall -o trfs trfs.c

freebsd:
	gcc -DFREEBSD -Wall -o trfs trfs.c

trfs:
	make all

install: trfs
	install -o root -g wheel -m 755 trfs $(DESTDIR)$(prefix)/bin
	install -o root -g wheel -m 755 trf  $(DESTDIR)$(prefix)/bin

clean:
	rm trfs || true
