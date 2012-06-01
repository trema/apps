#!/usr/bin/perl
#
# perl thisfile.pl > slice.dump
# rm slice.db
# sqlite3 slice.db < slice.dump
print<<_EOF_;
PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE slices (
       number          unsigned smallint not null,
       id              text not null,
       description     text,
       constraint number_unique unique (number) on conflict fail,
       constraint id_unique unique (id) on conflict fail
);
_EOF_
for ($i = 0; $i < 65535; $i++ ) {
	printf "INSERT INTO \"slices\" VALUES(%d,'test%d','slice%d');\n", $i, $i, $i;
}
print<<_EOF_;
CREATE TABLE bindings (
       type            unsigned tinyint not null,
       datapath_id     unsigned bigint,
       port            unsigned smallint,
       vid             unsigned smallint,
       mac             unsigned bigint,
       id              text not null,
       slice_number    unsigned smallint,
       constraint binding_unique unique (id,slice_number) on conflict fail
);
_EOF_
for ($i = 0; $i < 65535*2; $i++ ) {
	printf "INSERT INTO \"bindings\" VALUES(2,NULL,NULL,NULL,%d,'host%d',%d);\n", $i+1, $i+1, $i/2;
}
print<<_EOF_;
COMMIT;
_EOF_
