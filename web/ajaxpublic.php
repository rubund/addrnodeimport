<?php
include "mysql.php";
$conn = mysql_connect($dbhost,$dbuser,$dbpass) or die ('Error connecting to mysql');
mysql_select_db($dbname);
mysql_query("set names utf8");


if(isset($_GET['person'])){
    $result = mysql_query("insert into osmimportresp (kommunenummer, person, tid) values ('".mysql_escape_string($_GET['kommune'])."','".mysql_escape_string($_GET['person'])."',now());") or die('Mysql error');
    echo "OK";
}
elseif(isset($_GET['update'])){
    $result = mysql_query("insert into update_requests (kommunenummer, ip, tid) values ('".mysql_escape_string($_GET['kommune'])."','".mysql_escape_string($_SERVER['REMOTE_ADDR'])."',now());") or die('Mysql error');
    echo "OK";
}
?>
