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

    if(isset($_GET["now"])){
      if($_GET["now"] == 1){
        $result = mysql_query("update update_requests set tid=now() where kommunenummer = '".mysql_escape_string($_GET['kommune'])."' and ferdig=0;") or die('Mysql error');
        echo "OK";
      }
      else {
        $result = mysql_query("insert into update_requests (kommunenummer, ip, tid) values ('".mysql_escape_string($_GET['kommune'])."','".mysql_escape_string($_SERVER['REMOTE_ADDR'])."',addtime(now(), \"0:05:00\"));") or die('Mysql error');
        echo "OK";
      }
    }
}
?>
