create table slices (
       number	       unsigned smallint not null,
       id	       text not null,
       description     text,
       constraint number_unique unique (number) on conflict fail,
       constraint id_unique unique (id) on conflict fail
);

create table bindings (
       type            unsigned tinyint not null,
       datapath_id     unsigned bigint,
       port	       unsigned smallint,
       vid	       unsigned smallint,
       mac             unsigned bigint,
       id              text not null,
       slice_number    unsigned smallint,
       constraint binding_unique unique (id,slice_number) on conflict fail
);
