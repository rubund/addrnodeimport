drop table if exists nodes;
CREATE TABLE `nodes` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `dataset_id_first` int(11),
  `dataset_id_last` int(11),
  `muni_id` int(11) DEFAULT NULL,
  `addr_housenumber` int(11),
  `addr_street_id` int(11),
  `addr_postcode` int(11),
  `addr_city_id` int(11),
  `updated` datetime DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=UTF8;

drop table if exists node_streets;
CREATE TABLE `node_streets` (
    `street_id` int(11) NOT NULL AUTO_INCREMENT,
    `street_name` char(255) DEFAULT NULL,
    PRIMARY KEY (`street_id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=UTF8;


drop table if exists node_cities;
CREATE TABLE `node_cities` (
    `city_id` int(11) NOT NULL AUTO_INCREMENT,
    `city_name` varchar(255) DEFAULT NULL,
    PRIMARY KEY (`city_id`)
) ENGINE=InnoDB AUTO_INCREMENT=1 DEFAULT CHARSET=UTF8;
