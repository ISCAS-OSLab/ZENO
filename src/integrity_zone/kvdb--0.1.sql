-- noinspection SqlNoDataSourceInspectionForFile

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION kvdb" to load this file. \quit

--------------------------------------------------------------------------------
-- kv types --
--------------------------------------------------------------------------------

CREATE TYPE kv_int4;
CREATE TYPE kv_float4;
CREATE TYPE kv_timestamp;
CREATE TYPE kv_text;

CREATE FUNCTION generic_delete_trigger() RETURNS trigger
	AS 'MODULE_PATHNAME', 'generic_delete_trigger' LANGUAGE C;

CREATE FUNCTION kv_xact_init() RETURNS void
	AS 'MODULE_PATHNAME', 'kv_xact_init' LANGUAGE C;

CREATE FUNCTION kv_replay_log() RETURNS void
	AS 'MODULE_PATHNAME', 'kv_replay_log' LANGUAGE C;


SELECT kv_xact_init();

--------------------------------------------------------------------------------
-- kv int4 type --
--------------------------------------------------------------------------------

CREATE FUNCTION kv_int4_in(cstring) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_int4_out(kv_int4) RETURNS cstring
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_int4_recv(internal) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_int4_send(kv_int4) RETURNS bytea
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kv_int4 (
	INPUT		   = kv_int4_in,
	OUTPUT		   = kv_int4_out,
	receive 	   = kv_int4_recv,
	send 		   = kv_int4_send,
    INTERNALLENGTH = 8,
    ALIGNMENT      = double,
    PASSEDBYVALUE,
	STORAGE		   = PLAIN
);

CREATE FUNCTION kv_int4_eq(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR = (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_eq,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel,
	MERGES
);

CREATE FUNCTION kv_int4_ne(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <> (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_ne,
	COMMUTATOR = '<>',
	NEGATOR = '=',
	RESTRICT = neqsel,
	JOIN = neqjoinsel
);

CREATE FUNCTION kv_int4_lt(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR < (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_lt,
	COMMUTATOR = > ,
	NEGATOR = >= ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_int4_le(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <= (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_le,
	COMMUTATOR = >= ,
	NEGATOR = > ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_int4_gt(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR > (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_gt,
	COMMUTATOR = < ,
	NEGATOR = <= ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_int4_ge(kv_int4, kv_int4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR >= (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_ge,
	COMMUTATOR = <= ,
	NEGATOR = < ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_int4_cmp(kv_int4, kv_int4) RETURNS integer
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR CLASS btree_kv_int4_ops
	DEFAULT FOR TYPE kv_int4 USING btree AS
	OPERATOR	1   <  ,
	OPERATOR	2   <= ,
	OPERATOR	3   =  ,
	OPERATOR	4   >= ,
	OPERATOR	5   >  ,
	FUNCTION	1   kv_int4_cmp(kv_int4, kv_int4);

CREATE FUNCTION kv_int4_add(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR + (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_add
);

CREATE FUNCTION kv_int4_sub(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR - (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_sub
);

CREATE FUNCTION kv_int4_mult(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR * (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_mult
);

CREATE FUNCTION kv_int4_div(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR / (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_div
);

CREATE FUNCTION kv_int4_mod(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR % (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_mod
);

CREATE FUNCTION kv_int4_pow(kv_int4, kv_int4) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR ^ (
	LEFTARG = kv_int4,
	RIGHTARG = kv_int4,
	PROCEDURE = kv_int4_pow
);

CREATE FUNCTION kv_int4_sum_bulk(kv_int4[]) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE sum (kv_int4)
(
	sfunc = array_append,
	stype = kv_int4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_int4_sum_bulk  
);

CREATE FUNCTION kv_int4_avg_bulk(kv_int4[]) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE avg (kv_int4)
(
	sfunc = array_append,
	stype = kv_int4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_int4_avg_bulk
);

CREATE FUNCTION kv_int4_min_bulk(kv_int4[]) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE min (kv_int4)
(  
	sfunc = array_append,
	stype = kv_int4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_int4_min_bulk
);

CREATE FUNCTION kv_int4_max_bulk(kv_int4[])  RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE max (kv_int4)
(
	sfunc = array_append,
	stype = kv_int4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_int4_max_bulk
);

--------------------------------------------------------------------------------
-- kv float4 type --
--------------------------------------------------------------------------------

CREATE FUNCTION kv_float4_in(cstring) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_float4_out(kv_float4) RETURNS cstring
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kv_float4 (
	INPUT		  = kv_float4_in,
	OUTPUT		  = kv_float4_out,
    INTERNALLENGTH = 8,
    ALIGNMENT      = double,
    PASSEDBYVALUE,
	STORAGE		   = PLAIN
);

CREATE FUNCTION kv_float4_eq(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR = (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_eq,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel,
	MERGES
);

CREATE FUNCTION kv_float4_ne(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <> (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_ne,
	COMMUTATOR = '<>',
	NEGATOR = '=',
	RESTRICT = neqsel,
	JOIN = neqjoinsel
);

CREATE FUNCTION kv_float4_lt(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR < (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_lt,
	COMMUTATOR = > ,
	NEGATOR = >= ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_float4_le(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <= (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_le,
	COMMUTATOR = >= ,
	NEGATOR = > ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_float4_gt(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR > (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_gt,
	COMMUTATOR = < ,
	NEGATOR = <= ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_float4_ge(kv_float4, kv_float4) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR >= (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_ge,
	COMMUTATOR = <= ,
	NEGATOR = < ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_float4_cmp(kv_float4, kv_float4) RETURNS integer
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR CLASS btree_kv_float4_ops
	DEFAULT FOR TYPE kv_float4 USING btree AS
	OPERATOR	1   <  ,
	OPERATOR	2   <= ,
	OPERATOR	3   =  ,
	OPERATOR	4   >= ,
	OPERATOR	5   >  ,
	FUNCTION	1   kv_float4_cmp(kv_float4, kv_float4);

CREATE FUNCTION kv_float4_add(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR + (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_add
);

CREATE FUNCTION kv_float4_sub(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR - (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_sub
);

CREATE FUNCTION kv_float4_mult(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR * (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_mult
);

CREATE FUNCTION kv_float4_div(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR / (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_div
);

CREATE FUNCTION kv_float4_mod(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR % (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_mod
);

CREATE FUNCTION kv_float4_pow(kv_float4, kv_float4) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR ^ (
	LEFTARG = kv_float4,
	RIGHTARG = kv_float4,
	PROCEDURE = kv_float4_pow
);

CREATE FUNCTION kv_float4_avg_bulk(kv_float4[]) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE avg (kv_float4)
(
	sfunc = array_append,
	stype = kv_float4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_float4_avg_bulk
);

CREATE FUNCTION kv_float4_sum_bulk(kv_float4[]) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE sum (kv_float4)
(
	sfunc = array_append,
	stype = kv_float4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_float4_sum_bulk  
);

CREATE FUNCTION kv_float4_min_bulk(kv_float4[]) RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE min (kv_float4)
(  
	sfunc = array_append,
	stype = kv_float4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_float4_min_bulk
);

CREATE FUNCTION kv_float4_max_bulk(kv_float4[])  RETURNS kv_float4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE AGGREGATE max (kv_float4)
(
	sfunc = array_append,
	stype = kv_float4[],
	PARALLEL = SAFE,
	COMBINEFUNC = array_cat,
	finalfunc = kv_float4_max_bulk
);

--------------------------------------------------------------------------------
-- kv timestamp type --
--------------------------------------------------------------------------------

CREATE FUNCTION kv_timestamp_in(cstring) RETURNS kv_timestamp
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_timestamp_out(kv_timestamp) RETURNS cstring
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kv_timestamp (
	INPUT          = kv_timestamp_in,
	OUTPUT         = kv_timestamp_out,
    INTERNALLENGTH = 8,
    ALIGNMENT      = double,
    PASSEDBYVALUE,
	STORAGE		   = PLAIN
);

CREATE FUNCTION kv_timestamp_eq(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR = (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_eq,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel,
	MERGES
);

CREATE FUNCTION kv_timestamp_ne(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <> (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_ne,
	COMMUTATOR = '<>',
	NEGATOR = '=',
	RESTRICT = neqsel,
	JOIN = neqjoinsel
);

CREATE FUNCTION kv_timestamp_lt(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR < (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_lt,
	COMMUTATOR = > ,
	NEGATOR = >= ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_timestamp_le(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <= (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_le,
	COMMUTATOR = >= ,
	NEGATOR = > ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_timestamp_gt(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR > (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_gt,
	COMMUTATOR = < ,
	NEGATOR = <= ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_timestamp_ge(kv_timestamp, kv_timestamp) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR >= (
	LEFTARG = kv_timestamp,
	RIGHTARG = kv_timestamp,
	PROCEDURE = kv_timestamp_ge,
	COMMUTATOR = <= ,
	NEGATOR = < ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_timestamp_cmp(kv_timestamp, kv_timestamp) RETURNS integer
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR CLASS btree_kv_timestamp_ops
	DEFAULT FOR TYPE kv_timestamp USING btree AS
	OPERATOR	1   <  ,
	OPERATOR	2   <= ,
	OPERATOR	3   =  ,
	OPERATOR	4   >= ,
	OPERATOR	5   >  ,
	FUNCTION	1   kv_timestamp_cmp(kv_timestamp, kv_timestamp);

CREATE FUNCTION pg_catalog.date_part(text, kv_timestamp) RETURNS kv_int4
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

--------------------------------------------------------------------------------
-- kv string type --
--------------------------------------------------------------------------------

CREATE FUNCTION kv_text_in(cstring) RETURNS kv_text
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_text_out(kv_text) RETURNS cstring
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_text_recv(internal) RETURNS kv_text
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE FUNCTION kv_text_send(kv_text) RETURNS bytea
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kv_text (
	INPUT		   = kv_text_in,
	OUTPUT		   = kv_text_out,
	receive 	   = kv_text_recv,
	send 		   = kv_text_send,
    INTERNALLENGTH = 8,
    ALIGNMENT      = double,
    PASSEDBYVALUE,
	STORAGE		   = PLAIN
);

CREATE FUNCTION kv_text_eq(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR = (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_eq,
	COMMUTATOR = '=',
	NEGATOR = '<>',
	RESTRICT = eqsel,
	JOIN = eqjoinsel,
	MERGES
);

CREATE FUNCTION kv_text_ne(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <> (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_ne,
	COMMUTATOR = '<>',
	NEGATOR = '=',
	RESTRICT = neqsel,
	JOIN = neqjoinsel
);

CREATE FUNCTION kv_text_lt(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR < (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_lt,
	COMMUTATOR = > ,
	NEGATOR = >= ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_text_le(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR <= (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_le,
	COMMUTATOR = >= ,
	NEGATOR = > ,
	RESTRICT = scalarltsel,
	JOIN = scalarltjoinsel
);

CREATE FUNCTION kv_text_gt(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR > (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_gt,
	COMMUTATOR = < ,
	NEGATOR = <= ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_text_ge(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR >= (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_ge,
	COMMUTATOR = <= ,
	NEGATOR = < ,
	RESTRICT = scalargtsel,
	JOIN = scalargtjoinsel
);

CREATE FUNCTION kv_text_cmp(kv_text, kv_text) RETURNS integer
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR CLASS btree_kv_text_ops
	DEFAULT FOR TYPE kv_text USING btree AS
	OPERATOR	1   <  ,
	OPERATOR	2   <= ,
	OPERATOR	3   =  ,
	OPERATOR	4   >= ,
	OPERATOR	5   >  ,
	FUNCTION	1   kv_text_cmp(kv_text, kv_text);

CREATE FUNCTION kv_text_concatenate(kv_text, kv_text) RETURNS kv_text
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR || (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_concatenate
);

CREATE FUNCTION kv_text_like(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR ~~ (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_like
);

CREATE FUNCTION kv_text_notlike(kv_text, kv_text) RETURNS boolean
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
CREATE OPERATOR !~~ (
	LEFTARG = kv_text,
	RIGHTARG = kv_text,
	PROCEDURE = kv_text_notlike
);

CREATE FUNCTION pg_catalog.substring(kv_text, kv_int4, kv_int4) RETURNS kv_text
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION kv_int4(int4) RETURNS kv_int4
    AS 'MODULE_PATHNAME', 'int4_to_kv_int4' LANGUAGE C STRICT IMMUTABLE;
CREATE FUNCTION kv_int4(int8) RETURNS kv_int4
    AS 'MODULE_PATHNAME', 'int8_to_kv_int4' LANGUAGE C STRICT IMMUTABLE;

CREATE CAST (int4 AS kv_int4)
    WITH FUNCTION kv_int4(int4) AS ASSIGNMENT;
CREATE CAST (int8 AS kv_int4)
    WITH FUNCTION kv_int4(int8) AS ASSIGNMENT;

CREATE FUNCTION kv_float4(float4) RETURNS kv_float4
    AS 'MODULE_PATHNAME', 'float4_to_kv_float4' LANGUAGE C STRICT IMMUTABLE;
CREATE FUNCTION kv_float4(double precision) RETURNS kv_float4
    AS 'MODULE_PATHNAME', 'double_to_kv_float4' LANGUAGE C STRICT IMMUTABLE;
CREATE FUNCTION kv_float4(numeric) RETURNS kv_float4
    AS 'MODULE_PATHNAME', 'numeric_to_kv_float4' LANGUAGE C STRICT IMMUTABLE;
CREATE FUNCTION kv_float4(int8) RETURNS kv_float4
    AS 'MODULE_PATHNAME', 'int8_to_kv_float4' LANGUAGE C STRICT IMMUTABLE;
CREATE FUNCTION kv_float4(int4) RETURNS kv_float4
    AS 'MODULE_PATHNAME', 'int4_to_kv_float4' LANGUAGE C STRICT IMMUTABLE;

CREATE CAST (float4 AS kv_float4)
    WITH FUNCTION kv_float4(float4) AS ASSIGNMENT;
CREATE CAST (double precision AS kv_float4)
    WITH FUNCTION kv_float4(double precision) AS ASSIGNMENT;
CREATE CAST (numeric AS kv_float4)
    WITH FUNCTION kv_float4(numeric) AS ASSIGNMENT;
CREATE CAST (int8 AS kv_float4)
    WITH FUNCTION kv_float4(int8) AS ASSIGNMENT;
CREATE CAST (int4 AS kv_float4)
    WITH FUNCTION kv_float4(int4) AS ASSIGNMENT;

CREATE FUNCTION median(kv_float4[]) RETURNS kv_float4 AS
$$
	WITH q AS (
		SELECT val
		FROM unnest($1) val
		WHERE val IS NOT NULL
		ORDER BY 1
	),
	cnt AS (
		SELECT COUNT(*) AS c FROM q
	)
	SELECT AVG(val)::kv_float4
	FROM (
		SELECT val FROM q
		LIMIT  2 - MOD((SELECT c FROM cnt), 2)
		OFFSET GREATEST(CEIL((SELECT c FROM cnt) / 2.0) - 1,0)
	) q2;
$$
LANGUAGE SQL IMMUTABLE;

CREATE AGGREGATE median(kv_float4) (
	SFUNC = array_append,
	STYPE = kv_float4[],
	FINALFUNC = median,
	INITCOND = '{}'
);
