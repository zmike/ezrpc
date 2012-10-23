-- phpMyAdmin SQL Dump
-- version 2.11.10
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: May 24, 2010 at 01:43 PM
-- Server version: 5.0.77
-- PHP Version: 5.1.6

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `zentific`
--

-- --------------------------------------------------------

--
-- Table structure for table `alerts`
--

CREATE TABLE IF NOT EXISTS `alerts` (
  `id` int(11) NOT NULL auto_increment,
  `uid` int(11) NOT NULL default '-1',
  `acknowledged` tinyint(1) NOT NULL default '0',
  `deleted` tinyint(1) NOT NULL default '0',
  `message` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;

--
-- Dumping data for table `alerts`
--

INSERT INTO `alerts` (`id`, `uid`, `acknowledged`, `deleted`, `message`) VALUES
(1, 14, 0, 0, 'Example alert that might show up  on your homepage.');

-- --------------------------------------------------------

--
-- Table structure for table `config`
--

CREATE TABLE IF NOT EXISTS `config` (
  `id` int(11) NOT NULL auto_increment,
  `subsystem` enum('zensched','interface','zrpc','zpoll') NOT NULL default 'zrpc',
  `mid` int(11) NOT NULL default '-1',
  `uid` int(11) NOT NULL default '-1',
  `gid` int(11) NOT NULL default '-1',
  `vgid` int(11) NOT NULL default '-1',
  `ngid` int(11) NOT NULL default '-1',
  `vm` int(11) NOT NULL default '-1',
  `node` int(11) NOT NULL default '-1',
  `platform` int(11) NOT NULL default '-1',
  `key` varchar(256) NOT NULL default '',
  `value` varchar(256) NOT NULL default '',
  `valueBlob` blob NOT NULL default '',
  `role` int(11) NOT NULL default '-1',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=10 ;

--
-- Dumping data for table `config`
--

INSERT INTO `config` (`id`, `subsystem`, `mid`, `uid`, `gid`, `vgid`, `ngid`, `vm`, `node`, `platform`, `key`, `value`, `role`) VALUES
(1, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'stat_retention', '25', -1),
(2, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'nodeuser', 'default', -1),
(3, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'language', 'en_US', -1),
(4, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'max_memory_allocation', '8192', -1),
(5, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'max_vcpu_allocation', '4', -1),
(6, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'login_retry_timeout', '5', -1),
(7, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'max_login_failures', '5', -1),
(8, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'console_port_range', '9000-9100', -1),
(9, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'session_timeout', '15', -1),
(10, 'zrpc', -1, '-1', '-1', -1, -1, -1, -1, -1, 'proxied', 'true', -1);

-- --------------------------------------------------------

--
-- Table structure for table `diskconfig`
--

CREATE TABLE IF NOT EXISTS `diskconfig` (
  `disk` int(11) NOT NULL DEFAULT '-1',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `revision` int(11) NOT NULL DEFAULT '-1',
  `type` varchar(255) NOT NULL,
  `mapped_dev` varchar(255) NOT NULL,
  `mode` varchar(2) NOT NULL,
  `int_dev` varchar(255) NOT NULL,
  `ext_dev` varchar(1024) NOT NULL,
  `num_sectors` double NOT NULL,
  `capacity` double NOT NULL DEFAULT '0.0',
  `size_sector` int(11) NOT NULL,
  `block_size` int(11) NOT NULL,
  `start_sector` double NOT NULL,
  `partition_type` varchar(64) NOT NULL,
  KEY `timestamp` (`timestamp`),
  FOREIGN KEY (disk) REFERENCES disk(id) ON UPDATE CASCADE ON DELETE CASCADE,
  KEY `rev` (`disk`,`revision`),
  KEY `ext_dev` (`ext_dev`(1000))
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `diskconfig`
--


-- --------------------------------------------------------

--
-- Table structure for table `disks`
--

CREATE TABLE IF NOT EXISTS `disks` (
  `id` int(11) NOT NULL auto_increment,
  `storid` int(11) NOT NULL DEFAULT '-1',
  `enabled` tinyint(1) NOT NULL DEFAULT '0',
  `attached` tinyint(1) NOT NULL DEFAULT '0',
  `uuid` varchar(36) NOT NULL,
  `path` varchar(1024) NOT NULL,
  FOREIGN KEY (storid) REFERENCES storage(id) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (uuid) REFERENCES vms(uuid) ON UPDATE CASCADE ON DELETE CASCADE,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `disks`
--


-- --------------------------------------------------------

--
-- Table structure for table `diskstats`
--

CREATE TABLE IF NOT EXISTS `diskstats` (
  `disk` int(11) NOT NULL,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `rdreq` double NOT NULL default '0',
  `wrreq` double NOT NULL default '0',
  `ooreq` double NOT NULL default '0',
  KEY `timestamp` (`timestamp`),
  KEY `disk` (`disk`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `diskstats`
--


-- --------------------------------------------------------

--
-- Table structure for table `groups`
--

CREATE TABLE IF NOT EXISTS `groups` (
  `gid` int(11) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `desc` varchar(255) NOT NULL,
  `privilege` tinyint(1) NOT NULL default '4',
  PRIMARY KEY  (`gid`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `groups`
--


-- --------------------------------------------------------

--
-- Table structure for table `groupvms`
--

CREATE TABLE IF NOT EXISTS `groupvms` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `desc` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `groupvms`
--


-- --------------------------------------------------------

--
-- Table structure for table `jobs`
--

CREATE TABLE IF NOT EXISTS `jobs` (
  `id` bigint(20) NOT NULL auto_increment,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `uid` int(11) NOT NULL default '-1',
  `status` int(11) NOT NULL default '-1',
  `module_name` varchar(256) NOT NULL,
  `module_function` varchar(256) NOT NULL,
  `dependencies` varchar(256) NOT NULL DEFAULT "",
  `input_string` varchar(1024) NOT NULL DEFAULT "",
  `output_string` varchar(1024) NOT NULL DEFAULT "",
  `target_vm` char(36) NOT NULL,
  `target_node` char(36) NOT NULL,
  `target_host` varchar(255) NOT NULL,
  `extra` varchar(255) NOT NULL,
  `time_started` int(11) NOT NULL default '-1',
  `time_finished` int(11) NOT NULL default '-1',
  `return_value` int(11) NOT NULL default '-1',
  `pid` int(11) NOT NULL default '-1',
  `jobspeed` varchar(10) NOT NULL,
  `cron` varchar(255) NOT NULL,
  `nexttime` bigint(20) NOT NULL default '0',
  `croncount` int(11) NOT NULL default '-1',
  `faulttolerance` smallint(6) NOT NULL default '1',
  `scheduler` int(11) NOT NULL default '-1',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `jobs`
--


-- --------------------------------------------------------

--
-- Table structure for table `languages`
--

CREATE TABLE IF NOT EXISTS `languages` (
  `id` int(11) NOT NULL auto_increment,
  `language` varchar(8) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `language` (`language`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;

--
-- Dumping data for table `languages`
--

INSERT INTO `languages` (`id`, `language`) VALUES
(1, 'en_US');

-- --------------------------------------------------------

--
-- Table structure for table `logs`
--

CREATE TABLE IF NOT EXISTS `logs` (
  `id` int(11) NOT NULL auto_increment,
  `uid` int(11) NOT NULL,
  `vm` int(11) NOT NULL default '-1',
  `session_id` varchar(36) NOT NULL,
  `clientip` varchar(16) NOT NULL,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `uuid` varchar(36) NOT NULL,
  `severity` enum('info','warning','error','alert') NOT NULL default 'info',
  `message` varchar(1023) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `logs`
--


-- --------------------------------------------------------

--
-- Table structure for table `membership`
--

CREATE TABLE IF NOT EXISTS `membership` (
  `gid` int(11) NOT NULL,
  `uid` int(11) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `membership`
--


-- --------------------------------------------------------

--
-- Table structure for table `membershipvm`
--

CREATE TABLE IF NOT EXISTS `membershipvm` (
  `gid` int(11) NOT NULL,
  `uuid` varchar(36) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `membershipvm`
--


-- --------------------------------------------------------

--
-- Table structure for table `nodechildren`
--

CREATE TABLE IF NOT EXISTS `nodechildren` (
  `vm` int(11) NOT NULL,
  `node` int(11) NOT NULL,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  UNIQUE KEY `family` (`vm`,`node`),
  KEY `vm` (`vm`),
  KEY `node` (`node`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `nodechildren`
--


-- --------------------------------------------------------

--
-- Table structure for table `nodeconfig`
--

CREATE TABLE IF NOT EXISTS `nodeconfig` (
  `node` int(11) NOT NULL DEFAULT '-1',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `revision` int(11) NOT NULL DEFAULT '-1',
  `kernel` varchar(255) NOT NULL,
  `os` varchar(255) NOT NULL,
  `architecture` varchar(24) NOT NULL,
  `hostname` varchar(255) NOT NULL,
  `address` varchar(15) NOT NULL,
  `domainname` varchar(255) NOT NULL,
  `platform` varchar(255) NOT NULL,
  `platformver` varchar(255) NOT NULL,
  `platformbuild` varchar(255) NOT NULL,
  `platformfull` varchar(255) NOT NULL,
  `platformvendor` varchar(255) NOT NULL,
  `num_cpus` int(11) NOT NULL,
  `cores_per_socket` int(11) NOT NULL,
  `threads_per_core` int(11) NOT NULL,
  `num_cpu_nodes` int(11) NOT NULL,
  `sockets_per_node` int(11) NOT NULL,
  `cpu_mhz` int(11) NOT NULL,
  `total_phys_mem` int(11) NOT NULL,
  `control_mechanism` varchar(64) NOT NULL,
  `default_vnc_pass` varchar(255) NOT NULL,
  `hvm` int(1) NOT NULL default '0',
  `capabilities` varchar(256) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `nodeconfig`
--


-- --------------------------------------------------------

--
-- Table structure for table `nodes`
--

CREATE TABLE IF NOT EXISTS `nodes` (
  `id` int(11) NOT NULL auto_increment,
  `uuid` varchar(36) NOT NULL,
  `enabled` int(1) NOT NULL default '0',
  `controlmodule` varchar(256) NOT NULL,
  `configuredhost` varchar(256) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `nodes`
--


-- --------------------------------------------------------

--
-- Table structure for table `nodestats`
--

CREATE TABLE IF NOT EXISTS `nodestats` (
  `node` int(11) NOT NULL,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `iowait` double NOT NULL,
  `loadavg` varchar(24) NOT NULL,
  `uptime` int(11) NOT NULL,
  `cpupct` double NOT NULL,
  `intr` int(11) NOT NULL,
  `ctxts` int(11) NOT NULL,
  `num_procs` int(11) NOT NULL,
  `mem_free` int(11) NOT NULL,
  `mem_total` int(11) NOT NULL,
  `mem_shared` int(11) NOT NULL,
  `mem_buffered` int(11) NOT NULL,
  `swap_total` int(11) NOT NULL,
  `swap_free` int(11) NOT NULL,
  `free_phys_mem` double NOT NULL,
  `total_phys_mem` double NOT NULL,
  KEY `timestamp` (`timestamp`),
  KEY `node` (`node`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `nodestats`
--


-- --------------------------------------------------------

--
-- Table structure for table `ownershipnode`
--

CREATE TABLE IF NOT EXISTS `ownershipnode` (
  `uid` int(11) NOT NULL default '-1',
  `gid` int(11) NOT NULL default '-1',
  `node` int(11) NOT NULL default '-1',
  `privileges` int(1) NOT NULL default '4'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `ownershipnode`
--


-- --------------------------------------------------------

--
-- Table structure for table `ownershipvm`
--

CREATE TABLE IF NOT EXISTS `ownershipvm` (
  `uid` int(11) NOT NULL default '-1',
  `gid` int(11) NOT NULL default '-1',
  `vm` int(11) NOT NULL default '-1',
  `privileges` int(1) NOT NULL default '4'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `ownershipvm`
--


-- --------------------------------------------------------

--
-- Table structure for table `controlmodules`
--

CREATE TABLE IF NOT EXISTS `controlmodules` (
  `id` int(11) NOT NULL auto_increment,
  `platformid` int(11) NOT NULL,
  `scheduler` int(11) NOT NULL,
  `name` varchar(256) NOT NULL,
  `description` varchar(256) NOT NULL,
  `enabled` int(1) NOT NULL default '0',
  `controlstorage` tinyint(1) NOT NULL DEFAULT '-1',
  `controlnetwork` tinyint(1) NOT NULL DEFAULT '-1',
  `controlplatform` tinyint(1) NOT NULL DEFAULT '-1',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `controlmodules`
--


-- --------------------------------------------------------

--
-- Table structure for table `platforms`
--

CREATE TABLE IF NOT EXISTS `platforms` (
  `id` int(11) NOT NULL auto_increment,
  `scheduler` int(11) NOT NULL,
  `name` varchar(255) NOT NULL,
  `version` varchar(255) NOT NULL,
  `vendor` varchar(256) NOT NULL,
  `build` varchar(256) NOT NULL,
  `mechanism` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL,
  `enabled` int(1) NOT NULL default '0',
  `features` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `platforms`
--


-- --------------------------------------------------------

--
-- Table structure for table `plugins`
--

CREATE TABLE IF NOT EXISTS `plugins` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL,
  `frontend` int(1) NOT NULL default '0',
  `xmlrpc` int(1) NOT NULL default '0',
  `scheduler` int(1) NOT NULL default '0',
  `poller` int(1) NOT NULL default '0',
  `enabled` int(1) NOT NULL default '0',
  `minpriv` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=6 ;

--
-- Dumping data for table `plugins`
--

INSERT INTO `plugins` (`id`, `name`, `description`, `frontend`, `xmlrpc`, `scheduler`, `poller`, `enabled`, `minpriv`) VALUES
(2, 'Vms', 'Manage and view virtual machines', 1, 0, 0, 0, 1, 0),
(3, 'Nodes', 'Manage and view physical nodes', 1, 1, 1, 1, 1, 0),
(4, 'Users', 'User Management and Preferences', 1, 0, 0, 0, 1, 0),
(5, 'Networks', 'Manage network backends', 1, 0, 0, 0, 1, 0),
(6, 'Storage', 'Manage storage backends', 1, 0, 0, 0, 1, 0),
(7, 'Resource Pools', 'Manage resource pools', 1, 0, 0, 0, 1, 0),
(8, 'Configuration', 'Global Config and Module management', 1, 0, 0, 0, 1, 0);

-- --------------------------------------------------------

--
-- Table structure for table `privileges`
--

CREATE TABLE IF NOT EXISTS `privileges` (
  `privilege` tinyint(1) NOT NULL default '4',
  `name` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `privileges`
--

INSERT INTO `privileges` (`privilege`, `name`, `description`) VALUES
(0, 'superuser', 'owner of machine, can do anything, see all virtual machines'),
(1, 'maintenance', 'temporarily assigned to an admin for the purpose of fixing a VM, etc. same privs as admin for any assigned vms'),
(2, 'owner', 'The ultimate vm owner. same access privileges as admin, but can alter ownership of vm.'),
(3, 'admin', 'can control all aspects of any machines they own'),
(4, 'read-only admin', 'can only view information about the machines they own, these can be added by admin-level users to their account'),
(5, 'none', 'no privs whatsoever, placeholder only, expired or not-yet-activated accounts, etc'),
(6, 'node', 'for nodes posting data to xmlrpc');

-- --------------------------------------------------------

--
-- Table structure for table `revisions`
--

CREATE TABLE IF NOT EXISTS `revisions` (
  `id` int(11) NOT NULL auto_increment,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `uuid` varchar(36) NOT NULL,
  `vm` int(11) NOT NULL default '0',
  `node` int(11) NOT NULL default '0',
  `vif` int(11) NOT NULL default '0',
  `disk` int(11) NOT NULL default '0',
  `uid` int(11) NOT NULL default '-1',
  `name` varchar(255) NOT NULL,
  `description` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `timestamp` (`timestamp`),
  KEY `uid` (`uid`),
  KEY `name` (`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `revisions`
--


-- --------------------------------------------------------

--
-- Table structure for table `sessions`
--

CREATE TABLE IF NOT EXISTS `sessions` (
  `id` int(11) NOT NULL auto_increment,
  `session_id` varchar(36) NOT NULL,
  `authenticated` tinyint(1) NOT NULL default '0',
  `uid` int(9) NOT NULL default '0',
  `host` varchar(255) NOT NULL,
  `timestamp` int(11) NOT NULL default '0',
  `referrer` varchar(255) NOT NULL,
  `failures` int(11) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `session_id` (`session_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `networks`
--

CREATE TABLE IF NOT EXISTS `networks` (
  `id` int(11) NOT NULL auto_increment,
  `mid` int(11) NOT NULL DEFAULT '-1',
  `name` varchar(255) NOT NULL,
  `host` varchar(255) NOT NULL,
  `type` varchar(255) NOT NULL,
  `vlans` varchar(255) NOT NULL,
  `network` varchar(255) NOT NULL,
  `netmask` varchar(255) NOT NULL,
  `gateway` varchar(255) NOT NULL,
  `interface` varchar(255) NOT NULL,
  `mac` varchar(17) NOT NULL,
  UNIQUE KEY `name` (`name`),
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `networks`
--




-- --------------------------------------------------------

--
-- Table structure for table `storagenodes`
--

CREATE TABLE IF NOT EXISTS `storagenodes` (
  `node` int(11) NOT NULL DEFAULT '-1',
  `storage` int(11) NOT NULL DEFAULT '-1',
  FOREIGN KEY (node) REFERENCES nodes(id) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (storage) REFERENCES storage(id) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `storagenodes`
--




-- --------------------------------------------------------

--
-- Table structure for table `networksnodes`
--

CREATE TABLE IF NOT EXISTS `networksnodes` (
  `node` int(11) NOT NULL DEFAULT '-1',
  `network` int(11) NOT NULL DEFAULT '-1',
  FOREIGN KEY (node) REFERENCES nodes(id) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (network) REFERENCES networks(id) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `networksnodes`
--



-- --------------------------------------------------------

--
-- Table structure for table `storage`
--

CREATE TABLE IF NOT EXISTS `storage` (
  `id` int(11) NOT NULL auto_increment,
  `mid` int(11) NOT NULL DEFAULT '-1',
  `name` varchar(255) NOT NULL,
  `host` varchar(255) NOT NULL,
  `type` varchar(255) NOT NULL,
  `path` varchar(1024) NOT NULL,
  `mode` varchar(5) NOT NULL DEFAULT 'b',
  `capacity` int(11) NOT NULL DEFAULT '-1',
  `freespace` int(11) NOT NULL DEFAULT '-1',
  UNIQUE KEY `name` (`name`),
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `storage`
--


-- --------------------------------------------------------

--
-- Table structure for table `users`
--

CREATE TABLE IF NOT EXISTS `users` (
  `uid` int(11) NOT NULL auto_increment,
  `username` varchar(255) NOT NULL,
  `givenname` varchar(255) NOT NULL,
  `hash` varchar(255) NOT NULL,
  `salt` varchar(4) NOT NULL,
  `active` int(11) NOT NULL default '0',
  `registered` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `lastlogin` timestamp NOT NULL default '0000-00-00 00:00:00',
  `type` int(11) NOT NULL default '4',
  `lang` int(11) NOT NULL default '0',
  `email` varchar(255) NOT NULL,
  PRIMARY KEY  (`uid`),
  UNIQUE KEY `username` (`username`),
  KEY `active` (`active`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=34 ;

--
-- Dumping data for table `users`
--

INSERT INTO `users` (`uid`, `username`, `hash`, `salt`, `active`, `registered`, `type`, `lang`, `email`) VALUES
(0, 'admin', '', '', 1, '2008-11-21 12:11:31', 0, 0, '');

-- --------------------------------------------------------

--
-- Table structure for table `vifconfig`
--

CREATE TABLE IF NOT EXISTS `vifconfig` (
  `vif` int(11) NOT NULL DEFAULT '-1',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `revision` int(11) NOT NULL DEFAULT '-1',
  `type` enum('netfront','ioemu') NOT NULL default 'netfront',
  `mac` varchar(17) NOT NULL,
  `bridge` varchar(64) NOT NULL,
  `ip` varchar(15) NOT NULL,
  `netmask` varchar(15) NOT NULL,
  `gateway` varchar(15) NOT NULL,
  `broadcast` varchar(15) NOT NULL,
  `mtu` int(11) NOT NULL DEFAULT '1500',
  `name` varchar(128) NOT NULL,
  `script` varchar(255) NOT NULL,
  `backend` varchar(32) NOT NULL,
  `model` varchar(16) NOT NULL default '',
  FOREIGN KEY (vif) REFERENCES vif(id) ON UPDATE CASCADE ON DELETE CASCADE,
  KEY `timestamp` (`timestamp`),
  KEY `rev` (`vif`,`revision`),
  KEY `mac` (`mac`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vifconfig`
--


-- --------------------------------------------------------

--
-- Table structure for table `vifs`
--

CREATE TABLE IF NOT EXISTS `vifs` (
  `id` int(11) NOT NULL auto_increment,
  `nw` int(11) NOT NULL DEFAULT '-1',
  `mac` varchar(18) NOT NULL DEFAULT "00:00:00:00:00:00",
  `uuid` varchar(36) NOT NULL,
  FOREIGN KEY (nw) REFERENCES networks(id) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (uuid) REFERENCES vms(uuid) ON UPDATE CASCADE ON DELETE CASCADE,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `vifs`
--


-- --------------------------------------------------------

--
-- Table structure for table `vifstats`
--

CREATE TABLE IF NOT EXISTS `vifstats` (
  `vif` int(11) NOT NULL default '0',
  `timestamp` timestamp NULL default CURRENT_TIMESTAMP,
  `rxpackets` double NOT NULL default '0',
  `rxbytes` double NOT NULL default '0',
  `rxdrop` double NOT NULL default '0',
  `rxerr` double NOT NULL default '0',
  `txpackets` double NOT NULL default '0',
  `txbytes` double NOT NULL default '0',
  `txdrop` double NOT NULL default '0',
  `txerr` double NOT NULL default '0',
  KEY `timestamp` (`timestamp`),
  KEY `vif` (`vif`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vifstats`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmconfig`
--

CREATE TABLE IF NOT EXISTS `vmconfig` (
  `vm` int(11) NOT NULL default '-1',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `revision` int(11) NOT NULL DEFAULT '-1',
  `vncpasswd` varchar(255) NOT NULL,
  `vcpus` int(11) NOT NULL,
  `kernel` varchar(1024) NOT NULL,
  `ramdisk` varchar(1024) NOT NULL,
  `cmdline` varchar(255) NOT NULL,
  `name` varchar(255) NOT NULL,
  `type` enum('host','para','full','hybrid','other') NOT NULL,
  `os` varchar(255) NOT NULL,
  `mem` int(11) NOT NULL,
  `maxmem` int(11) NOT NULL,
  `vnc` tinyint(4) NOT NULL,
  `vncport` int(11) NOT NULL,
  `sdl` tinyint(4) NOT NULL,
  `pae` tinyint(4) NOT NULL,
  `acpi` tinyint(4) NOT NULL,
  `apic` tinyint(4) NOT NULL,
  `backend` tinyint(4) NOT NULL,
  `bootloader` varchar(255) NOT NULL,
  `bootloaderargs` varchar(255) NOT NULL,
  `sound` varchar(255) NOT NULL,
  `boot` varchar(255) NOT NULL,
  `device_model` varchar(255) NOT NULL,
  `on_reboot` varchar(32) NOT NULL default 'preserve',
  `on_crash` varchar(32) NOT NULL default 'preserve',
  `on_poweroff` varchar(32) NOT NULL default 'preserve',
  `monitorstate` tinyint(1) NOT NULL default '0',
  `zon_reboot` varchar(128) NOT NULL default 'reboot',
  `zon_crash` varchar(128) NOT NULL default 'reboot',
  `zon_poweroff` varchar(128) NOT NULL default 'destroy',
  KEY `vm` (`vm`),
  KEY `rev` (`vm`,`revision`),
  KEY `timestamp` (`timestamp`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vmconfig`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmconsolelocks`
--

CREATE TABLE IF NOT EXISTS `vmconsolelocks` (
  `vm` int(11) NOT NULL default '0',
  `nodehost` varchar(64) NOT NULL,
  `session` int(11) NOT NULL default '0',
  `client` varchar(64) NOT NULL,
  `externalport` int(11) NOT NULL default '0',
  `tunnelport` int(11) NOT NULL default '0',
  `type` enum('vnc','text') NOT NULL,
  KEY `vm` (`vm`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vmconsolelocks`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmlogs`
--

CREATE TABLE IF NOT EXISTS `vmlogs` (
  `vm` int(11) NOT NULL default '-1',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `uid` int(11) NOT NULL,
  `severity` varchar(16) NOT NULL,
  `message` varchar(255) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vmlogs`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmnotes`
--

CREATE TABLE IF NOT EXISTS `vmnotes` (
  `vm` int(11) NOT NULL,
  `notes` longtext NOT NULL,
  PRIMARY KEY  (`vm`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vmnotes`
--


-- --------------------------------------------------------

--
-- Table structure for table `vms`
--

CREATE TABLE IF NOT EXISTS `vms` (
  `id` int(11) NOT NULL auto_increment,
  `uuid` varchar(36) NOT NULL,
  `needsreboot` int(1) NOT NULL default '0',
  `locked` int(1) NOT NULL default '0',
  `template` int(1) NOT NULL default '0',
  `zentifictools` tinyint(1) NOT NULL default '0',
  `ignored` int(1) NOT NULL default '0',
  `state` varchar(5) NOT NULL DEFAULT 's',
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `uuid` (`uuid`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 AUTO_INCREMENT=1 ;

--
-- Dumping data for table `vms`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmstats`
--

CREATE TABLE IF NOT EXISTS `vmstats` (
  `vm` int(11) NOT NULL,
  `timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP,
  `state` varchar(6) NOT NULL,
  `uptime` int(11) NOT NULL,
  `cputime` double NOT NULL,
  `cpupct` double NOT NULL,
  `rxbw` int(11) NOT NULL,
  `txbw` int(11) NOT NULL,
  `domid` int(11) NOT NULL,
  KEY `timestamp` (`timestamp`),
  KEY `vm` (`vm`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `vmstats`
--


-- --------------------------------------------------------

--
-- Table structure for table `vmtemplates`
--

CREATE TABLE IF NOT EXISTS `vmtemplates` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(64) NOT NULL,
  `architecture` enum('32','32p','64') NOT NULL,
  `description` varchar(255) NOT NULL,
  `type` enum('pv','full','hybrid') NOT NULL,
  `uuid` varchar(36) NOT NULL,
  `platform` int(11) NOT NULL,
  UNIQUE KEY `name` (`name`),
  KEY `id` (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=3 ;

--
-- Dumping data for table `vmtemplates`
--

INSERT INTO `vmtemplates` (`id`, `name`, `architecture`, `description`, `type`, `uuid`, `platform`) VALUES
(2, 'Ubuntu 8.04', '64', 'Hardy paravirtual installer', 'pv', 'deadbeef-0000-0000-0000-000000000000', 0);


-- --------------------------------------------------------

--
-- Table structure for table `operatingsystems`
--

CREATE TABLE IF NOT EXISTS `operatingsystems` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `name` (`name`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=4 ;

--
-- Dumping data for table `operatingsystems`
--

INSERT INTO `operatingsystems` (`id`, `name`) VALUES
(1, 'Debian Linux 5.0'),
(2, 'RedHat Enterprise Linux 5.x'),
(3, 'Microsoft Windows 2003');


-- --------------------------------------------------------

--
-- Table structure for table `schedulers`
--

CREATE TABLE IF NOT EXISTS `schedulers` (
	`id` int(11) NOT NULL auto_increment,
	`hostname` varchar(256) NOT NULL DEFAULT "",
	`address` varchar(256) NOT NULL DEFAULT "",
	`port` int(5) NOT NULL DEFAULT '-1',
	PRIMARY KEY  (`id`),
	UNIQUE KEY `hostname` (`hostname`)
);

--
-- Table structure for table `schedulers`
--

