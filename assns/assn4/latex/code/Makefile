PROG=	secret
SRCS=	secret.c

DPADD+=	${LIBDRIVER} ${LIBSYS}
LDADD+=	-ldriver -lsys

MAN=

BINDIR?= /usr/sbin

.include <bsd.prog.mk>

up: secret
	service up /usr/src/drivers/secrets/secret -dev /dev/secret

down: secret	
	service down secret
