-- Load the ZENO PostgreSQL extension.
DROP EXTENSION IF EXISTS kvdb CASCADE;
CREATE EXTENSION kvdb;

-- Create a small table that uses all supported opaque types.
DROP TABLE IF EXISTS test;
CREATE TABLE test (i kv_int4, f kv_float4, s kv_text, t kv_timestamp);

-- Keep inserts visible to ZENO's temporal-partition trigger path.
CREATE TRIGGER tri BEFORE INSERT ON test
FOR EACH ROW EXECUTE PROCEDURE generic_delete_trigger(i, f, s, t);

-- Route deletes through ZENO's logical-delete trigger path.
CREATE TRIGGER tri3 AFTER DELETE ON test
FOR EACH ROW EXECUTE PROCEDURE generic_delete_trigger(i, f, s, t);

INSERT INTO test VALUES ('1'::kv_int4, '1.1'::kv_float4, 'OSDI'::kv_text, '2026-05-06'::kv_timestamp);

-- Verify that the encrypted values can be inserted and read back.
SELECT * FROM test;
