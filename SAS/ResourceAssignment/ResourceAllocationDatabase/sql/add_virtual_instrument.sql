-- resourceassignment password for testing on mcu005 is the same as the password on the president's luggage +6
-- psql resourceassignment -U resourceassignment -f add_virtual_instrument.sql -W
BEGIN;
INSERT INTO virtual_instrument.unit VALUES 
(0, 'rsp_channel_bit'),(1, 'bytes'),(2, 'rcu_board'),(3, 'bytes/second'),(4, 'cores');
INSERT INTO virtual_instrument.resource_type VALUES 
(0, 'rsp', 0), (1, 'tbb', 1), (2, 'rcu', 2), (3, 'bandwidth', 3), (4, 'processor', 4), (5, 'storage', 1);
INSERT INTO virtual_instrument.resource_group_type VALUES 
(0, 'instrument'),(1, 'cluster'),(2, 'station_group'),(3, 'station'),(4, 'node_group'),(5, 'node');
INSERT INTO virtual_instrument.resource_group VALUES 
(0, 'LOFAR', 0), (1, 'CEP4', 1),
(2, 'Cobalt', 1), (3, 'cpunode01', 5),
(4, 'cpunode02', 5),
(5, 'cpunode03', 5),
(6, 'cpunode04', 5),
(7, 'cpunode05', 5),
(8, 'cpunode06', 5),
(9, 'cpunode07', 5),
(10, 'cpunode08', 5),
(11, 'cpunode09', 5),
(12, 'cpunode10', 5),
(13, 'cpunode11', 5),
(14, 'cpunode12', 5),
(15, 'cpunode13', 5),
(16, 'cpunode14', 5),
(17, 'cpunode15', 5),
(18, 'cpunode16', 5),
(19, 'cpunode17', 5),
(20, 'cpunode18', 5),
(21, 'cpunode19', 5),
(22, 'cpunode20', 5),
(23, 'cpunode21', 5),
(24, 'cpunode22', 5),
(25, 'cpunode23', 5),
(26, 'cpunode24', 5),
(27, 'cpunode25', 5),
(28, 'cpunode26', 5),
(29, 'cpunode27', 5),
(30, 'cpunode28', 5),
(31, 'cpunode29', 5),
(32, 'cpunode30', 5),
(33, 'cpunode31', 5),
(34, 'cpunode32', 5),
(35, 'cpunode33', 5),
(36, 'cpunode34', 5),
(37, 'cpunode35', 5),
(38, 'cpunode36', 5),
(39, 'cpunode37', 5),
(40, 'cpunode38', 5),
(41, 'cpunode39', 5),
(42, 'cpunode40', 5),
(43, 'cpunode41', 5),
(44, 'cpunode42', 5),
(45, 'cpunode43', 5),
(46, 'cpunode44', 5),
(47, 'cpunode45', 5),
(48, 'cpunode46', 5),
(49, 'cpunode47', 5),
(50, 'cpunode48', 5),
(51, 'cpunode49', 5),
(52, 'cpunode50', 5);
INSERT INTO virtual_instrument.resource VALUES (0, 'bandwidth', 3),
(1, 'processor', 4),
(2, 'bandwidth', 3),
(3, 'processor', 4),
(4, 'bandwidth', 3),
(5, 'processor', 4),
(6, 'bandwidth', 3),
(7, 'processor', 4),
(8, 'bandwidth', 3),
(9, 'processor', 4),
(10, 'bandwidth', 3),
(11, 'processor', 4),
(12, 'bandwidth', 3),
(13, 'processor', 4),
(14, 'bandwidth', 3),
(15, 'processor', 4),
(16, 'bandwidth', 3),
(17, 'processor', 4),
(18, 'bandwidth', 3),
(19, 'processor', 4),
(20, 'bandwidth', 3),
(21, 'processor', 4),
(22, 'bandwidth', 3),
(23, 'processor', 4),
(24, 'bandwidth', 3),
(25, 'processor', 4),
(26, 'bandwidth', 3),
(27, 'processor', 4),
(28, 'bandwidth', 3),
(29, 'processor', 4),
(30, 'bandwidth', 3),
(31, 'processor', 4),
(32, 'bandwidth', 3),
(33, 'processor', 4),
(34, 'bandwidth', 3),
(35, 'processor', 4),
(36, 'bandwidth', 3),
(37, 'processor', 4),
(38, 'bandwidth', 3),
(39, 'processor', 4),
(40, 'bandwidth', 3),
(41, 'processor', 4),
(42, 'bandwidth', 3),
(43, 'processor', 4),
(44, 'bandwidth', 3),
(45, 'processor', 4),
(46, 'bandwidth', 3),
(47, 'processor', 4),
(48, 'bandwidth', 3),
(49, 'processor', 4),
(50, 'bandwidth', 3),
(51, 'processor', 4),
(52, 'bandwidth', 3),
(53, 'processor', 4),
(54, 'bandwidth', 3),
(55, 'processor', 4),
(56, 'bandwidth', 3),
(57, 'processor', 4),
(58, 'bandwidth', 3),
(59, 'processor', 4),
(60, 'bandwidth', 3),
(61, 'processor', 4),
(62, 'bandwidth', 3),
(63, 'processor', 4),
(64, 'bandwidth', 3),
(65, 'processor', 4),
(66, 'bandwidth', 3),
(67, 'processor', 4),
(68, 'bandwidth', 3),
(69, 'processor', 4),
(70, 'bandwidth', 3),
(71, 'processor', 4),
(72, 'bandwidth', 3),
(73, 'processor', 4),
(74, 'bandwidth', 3),
(75, 'processor', 4),
(76, 'bandwidth', 3),
(77, 'processor', 4),
(78, 'bandwidth', 3),
(79, 'processor', 4),
(80, 'bandwidth', 3),
(81, 'processor', 4),
(82, 'bandwidth', 3),
(83, 'processor', 4),
(84, 'bandwidth', 3),
(85, 'processor', 4),
(86, 'bandwidth', 3),
(87, 'processor', 4),
(88, 'bandwidth', 3),
(89, 'processor', 4),
(90, 'bandwidth', 3),
(91, 'processor', 4),
(92, 'bandwidth', 3),
(93, 'processor', 4),
(94, 'bandwidth', 3),
(95, 'processor', 4),
(96, 'bandwidth', 3),
(97, 'processor', 4),
(98, 'bandwidth', 3),
(99, 'processor', 4),
(100, 'cep4bandwidth', 3),
(101, 'cep4storage', 5);
INSERT INTO virtual_instrument.resource_to_resource_group VALUES (DEFAULT, 0, 3), (DEFAULT, 1, 3), (DEFAULT, 2, 4), (DEFAULT, 3, 4), (DEFAULT, 4, 5), (DEFAULT, 5, 5), (DEFAULT, 6, 6), (DEFAULT, 7, 6), (DEFAULT, 8, 7), (DEFAULT, 9, 7), (DEFAULT, 10, 8), (DEFAULT, 11, 8), (DEFAULT, 12, 9), (DEFAULT, 13, 9), (DEFAULT, 14, 10), (DEFAULT, 15, 10), (DEFAULT, 16, 11), (DEFAULT, 17, 11), (DEFAULT, 18, 12), (DEFAULT, 19, 12), (DEFAULT, 20, 13), (DEFAULT, 21, 13), (DEFAULT, 22, 14), (DEFAULT, 23, 14), (DEFAULT, 24, 15), (DEFAULT, 25, 15), (DEFAULT, 26, 16), (DEFAULT, 27, 16), (DEFAULT, 28, 17), (DEFAULT, 29, 17), (DEFAULT, 30, 18), (DEFAULT, 31, 18), (DEFAULT, 32, 19), (DEFAULT, 33, 19), (DEFAULT, 34, 20), (DEFAULT, 35, 20), (DEFAULT, 36, 21), (DEFAULT, 37, 21), (DEFAULT, 38, 22), (DEFAULT, 39, 22), (DEFAULT, 40, 23), (DEFAULT, 41, 23), (DEFAULT, 42, 24), (DEFAULT, 43, 24), (DEFAULT, 44, 25), (DEFAULT, 45, 25), (DEFAULT, 46, 26), (DEFAULT, 47, 26), (DEFAULT, 48, 27), (DEFAULT, 49, 27), (DEFAULT, 50, 28), (DEFAULT, 51, 28), (DEFAULT, 52, 29), (DEFAULT, 53, 29), (DEFAULT, 54, 30), (DEFAULT, 55, 30), (DEFAULT, 56, 31), (DEFAULT, 57, 31), (DEFAULT, 58, 32), (DEFAULT, 59, 32), (DEFAULT, 60, 33), (DEFAULT, 61, 33), (DEFAULT, 62, 34), (DEFAULT, 63, 34), (DEFAULT, 64, 35), (DEFAULT, 65, 35), (DEFAULT, 66, 36), (DEFAULT, 67, 36), (DEFAULT, 68, 37), (DEFAULT, 69, 37), (DEFAULT, 70, 38), (DEFAULT, 71, 38), (DEFAULT, 72, 39), (DEFAULT, 73, 39), (DEFAULT, 74, 40), (DEFAULT, 75, 40), (DEFAULT, 76, 41), (DEFAULT, 77, 41), (DEFAULT, 78, 42), (DEFAULT, 79, 42), (DEFAULT, 80, 43), (DEFAULT, 81, 43), (DEFAULT, 82, 44), (DEFAULT, 83, 44), (DEFAULT, 84, 45), (DEFAULT, 85, 45), (DEFAULT, 86, 46), (DEFAULT, 87, 46), (DEFAULT, 88, 47), (DEFAULT, 89, 47), (DEFAULT, 90, 48), (DEFAULT, 91, 48), (DEFAULT, 92, 49), (DEFAULT, 93, 49), (DEFAULT, 94, 50), (DEFAULT, 95, 50), (DEFAULT, 96, 51), (DEFAULT, 97, 51), (DEFAULT, 98, 52), (DEFAULT, 99, 52), (DEFAULT, 100, 3), (DEFAULT, 101, 3);
INSERT INTO resource_monitoring.resource_capacity VALUES (DEFAULT, 0, 53687091200, 53687091200), (DEFAULT, 1, 24, 24), (DEFAULT, 2, 53687091200, 53687091200), (DEFAULT, 3, 24, 24), (DEFAULT, 4, 53687091200, 53687091200), (DEFAULT, 5, 24, 24), (DEFAULT, 6, 53687091200, 53687091200), (DEFAULT, 7, 24, 24), (DEFAULT, 8, 53687091200, 53687091200), (DEFAULT, 9, 24, 24), (DEFAULT, 10, 53687091200, 53687091200), (DEFAULT, 11, 24, 24), (DEFAULT, 12, 53687091200, 53687091200), (DEFAULT, 13, 24, 24), (DEFAULT, 14, 53687091200, 53687091200), (DEFAULT, 15, 24, 24), (DEFAULT, 16, 53687091200, 53687091200), (DEFAULT, 17, 24, 24), (DEFAULT, 18, 53687091200, 53687091200), (DEFAULT, 19, 24, 24), (DEFAULT, 20, 53687091200, 53687091200), (DEFAULT, 21, 24, 24), (DEFAULT, 22, 53687091200, 53687091200), (DEFAULT, 23, 24, 24), (DEFAULT, 24, 53687091200, 53687091200), (DEFAULT, 25, 24, 24), (DEFAULT, 26, 53687091200, 53687091200), (DEFAULT, 27, 24, 24), (DEFAULT, 28, 53687091200, 53687091200), (DEFAULT, 29, 24, 24), (DEFAULT, 30, 53687091200, 53687091200), (DEFAULT, 31, 24, 24), (DEFAULT, 32, 53687091200, 53687091200), (DEFAULT, 33, 24, 24), (DEFAULT, 34, 53687091200, 53687091200), (DEFAULT, 35, 24, 24), (DEFAULT, 36, 53687091200, 53687091200), (DEFAULT, 37, 24, 24), (DEFAULT, 38, 53687091200, 53687091200), (DEFAULT, 39, 24, 24), (DEFAULT, 40, 53687091200, 53687091200), (DEFAULT, 41, 24, 24), (DEFAULT, 42, 53687091200, 53687091200), (DEFAULT, 43, 24, 24), (DEFAULT, 44, 53687091200, 53687091200), (DEFAULT, 45, 24, 24), (DEFAULT, 46, 53687091200, 53687091200), (DEFAULT, 47, 24, 24), (DEFAULT, 48, 53687091200, 53687091200), (DEFAULT, 49, 24, 24), (DEFAULT, 50, 53687091200, 53687091200), (DEFAULT, 51, 24, 24), (DEFAULT, 52, 53687091200, 53687091200), (DEFAULT, 53, 24, 24), (DEFAULT, 54, 53687091200, 53687091200), (DEFAULT, 55, 24, 24), (DEFAULT, 56, 53687091200, 53687091200), (DEFAULT, 57, 24, 24), (DEFAULT, 58, 53687091200, 53687091200), (DEFAULT, 59, 24, 24), (DEFAULT, 60, 53687091200, 53687091200), (DEFAULT, 61, 24, 24), (DEFAULT, 62, 53687091200, 53687091200), (DEFAULT, 63, 24, 24), (DEFAULT, 64, 53687091200, 53687091200), (DEFAULT, 65, 24, 24), (DEFAULT, 66, 53687091200, 53687091200), (DEFAULT, 67, 24, 24), (DEFAULT, 68, 53687091200, 53687091200), (DEFAULT, 69, 24, 24), (DEFAULT, 70, 53687091200, 53687091200), (DEFAULT, 71, 24, 24), (DEFAULT, 72, 53687091200, 53687091200), (DEFAULT, 73, 24, 24), (DEFAULT, 74, 53687091200, 53687091200), (DEFAULT, 75, 24, 24), (DEFAULT, 76, 53687091200, 53687091200), (DEFAULT, 77, 24, 24), (DEFAULT, 78, 53687091200, 53687091200), (DEFAULT, 79, 24, 24), (DEFAULT, 80, 53687091200, 53687091200), (DEFAULT, 81, 24, 24), (DEFAULT, 82, 53687091200, 53687091200), (DEFAULT, 83, 24, 24), (DEFAULT, 84, 53687091200, 53687091200), (DEFAULT, 85, 24, 24), (DEFAULT, 86, 53687091200, 53687091200), (DEFAULT, 87, 24, 24), (DEFAULT, 88, 53687091200, 53687091200), (DEFAULT, 89, 24, 24), (DEFAULT, 90, 53687091200, 53687091200), (DEFAULT, 91, 24, 24), (DEFAULT, 92, 53687091200, 53687091200), (DEFAULT, 93, 24, 24), (DEFAULT, 94, 53687091200, 53687091200), (DEFAULT, 95, 24, 24), (DEFAULT, 96, 53687091200, 53687091200), (DEFAULT, 97, 24, 24), (DEFAULT, 98, 53687091200, 53687091200), (DEFAULT, 99, 24, 24), (DEFAULT, 100, 85899345920, 85899345920), (DEFAULT, 101, 2254857830400, 2254857830400);
INSERT INTO resource_monitoring.resource_availability VALUES (DEFAULT, 0, TRUE), (DEFAULT, 1, TRUE), (DEFAULT, 2, TRUE), (DEFAULT, 3, TRUE), (DEFAULT, 4, TRUE), (DEFAULT, 5, TRUE), (DEFAULT, 6, TRUE), (DEFAULT, 7, TRUE), (DEFAULT, 8, TRUE), (DEFAULT, 9, TRUE), (DEFAULT, 10, TRUE), (DEFAULT, 11, TRUE), (DEFAULT, 12, TRUE), (DEFAULT, 13, TRUE), (DEFAULT, 14, TRUE), (DEFAULT, 15, TRUE), (DEFAULT, 16, TRUE), (DEFAULT, 17, TRUE), (DEFAULT, 18, TRUE), (DEFAULT, 19, TRUE), (DEFAULT, 20, TRUE), (DEFAULT, 21, TRUE), (DEFAULT, 22, TRUE), (DEFAULT, 23, TRUE), (DEFAULT, 24, TRUE), (DEFAULT, 25, TRUE), (DEFAULT, 26, TRUE), (DEFAULT, 27, TRUE), (DEFAULT, 28, TRUE), (DEFAULT, 29, TRUE), (DEFAULT, 30, TRUE), (DEFAULT, 31, TRUE), (DEFAULT, 32, TRUE), (DEFAULT, 33, TRUE), (DEFAULT, 34, TRUE), (DEFAULT, 35, TRUE), (DEFAULT, 36, TRUE), (DEFAULT, 37, TRUE), (DEFAULT, 38, TRUE), (DEFAULT, 39, TRUE), (DEFAULT, 40, TRUE), (DEFAULT, 41, TRUE), (DEFAULT, 42, TRUE), (DEFAULT, 43, TRUE), (DEFAULT, 44, TRUE), (DEFAULT, 45, TRUE), (DEFAULT, 46, TRUE), (DEFAULT, 47, TRUE), (DEFAULT, 48, TRUE), (DEFAULT, 49, TRUE), (DEFAULT, 50, TRUE), (DEFAULT, 51, TRUE), (DEFAULT, 52, TRUE), (DEFAULT, 53, TRUE), (DEFAULT, 54, TRUE), (DEFAULT, 55, TRUE), (DEFAULT, 56, TRUE), (DEFAULT, 57, TRUE), (DEFAULT, 58, TRUE), (DEFAULT, 59, TRUE), (DEFAULT, 60, TRUE), (DEFAULT, 61, TRUE), (DEFAULT, 62, TRUE), (DEFAULT, 63, TRUE), (DEFAULT, 64, TRUE), (DEFAULT, 65, TRUE), (DEFAULT, 66, TRUE), (DEFAULT, 67, TRUE), (DEFAULT, 68, TRUE), (DEFAULT, 69, TRUE), (DEFAULT, 70, TRUE), (DEFAULT, 71, TRUE), (DEFAULT, 72, TRUE), (DEFAULT, 73, TRUE), (DEFAULT, 74, TRUE), (DEFAULT, 75, TRUE), (DEFAULT, 76, TRUE), (DEFAULT, 77, TRUE), (DEFAULT, 78, TRUE), (DEFAULT, 79, TRUE), (DEFAULT, 80, TRUE), (DEFAULT, 81, TRUE), (DEFAULT, 82, TRUE), (DEFAULT, 83, TRUE), (DEFAULT, 84, TRUE), (DEFAULT, 85, TRUE), (DEFAULT, 86, TRUE), (DEFAULT, 87, TRUE), (DEFAULT, 88, TRUE), (DEFAULT, 89, TRUE), (DEFAULT, 90, TRUE), (DEFAULT, 91, TRUE), (DEFAULT, 92, TRUE), (DEFAULT, 93, TRUE), (DEFAULT, 94, TRUE), (DEFAULT, 95, TRUE), (DEFAULT, 96, TRUE), (DEFAULT, 97, TRUE), (DEFAULT, 98, TRUE), (DEFAULT, 99, TRUE), (DEFAULT, 100, TRUE), (DEFAULT, 101, TRUE);
INSERT INTO virtual_instrument.resource_group_to_resource_group VALUES (DEFAULT, 0, NULL), (DEFAULT, 1, 0), (DEFAULT, 2, 0);
COMMIT;
