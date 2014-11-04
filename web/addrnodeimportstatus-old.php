<?php
include "kommunenummer.php";
header('Content-Type: text/html; charset=utf-8');
echo "<html><head><title>Adressenode-import-status for Norge</title>";
?>
<style>
p {font-family : "Arial";}
td {font-family : "Arial"; font-size : 11pt}
th {font-family : "Arial";}
h3 {font-family : "Arial";}
</style>
<?php
echo "</head>";
echo "<body>";
echo "<h3>Adressenode-import-status for Norge</h3>";
echo "<p>Sist oppdatert: <font color=\"green\">".date ("d. M Y H:i:s.", filemtime('reports/report_1943.txt'))."</font></p>";
echo "<table>";
echo "<tr><th>Kommunenr.</th><th>Kommune</th><th>#OSM</th><th>Bygn.</th><th>#Kartverket</th><th>Manglende</th><th>Ikke funnet</th><th>Ikke funnet POIer</th><th>Fullført</th><th>Status</th><th></th><th>Vei/veg-feil</th><th>Andre objekter med addr:</th><th>Mulige duplikater</th><th>Mangler postnr/by</th><th>Postnr/by feil</th><th>Bare husnr.</th><th>Oppdatert</th></tr>";
for($i=0;$i<2100;$i++){
    $kommunenummer = sprintf("%04d",$i);
    $path = sprintf("reports/report_%04d.txt",$i);
    //echo $path;
    if(file_exists($path)){
        $file = fopen($path,"r");
        $content = fread($file,10000);
        preg_match("/Existing:\s+(\d+)\s+New:\s+(\d+)\s+Missing:\s+(\d+)/",$content,$matches);
        $existing = $matches[1];
        $new = $matches[2];
        $missing = $matches[3];
        preg_match("/Existing:\s+(\d+)\s+New:\s+(\d+)\s+Missing:\s+(\d+)\s+Otherthings:\s+(\d+)\s+Duplicates:\s+(\d+)\s+Veivegfixes:\s+(\d+)/",$content,$matches);
        $otherthings = $matches[4];
        $duplicates = $matches[5];
        $veivegfixes = $matches[6];
        preg_match("/Buildings:\s+(\d+)/",$content,$matches);
        $buildings = $matches[1];
        preg_match("/Notmatched:\s+(\d+)/",$content,$matches);
        $notmatched = $matches[1];
        preg_match("/NotmatchedPOIs:\s+(\d+)/",$content,$matches);
        $notmatchedpois = $matches[1];
        $completeness = ($new-$missing)*1.0 / $new * 100.0;
        $linktoosm = "";
        if ($completeness > 90){
            $ledhtml = "<img src=\"greenled.png\" />";
            if ($missing != 0)
                $linktoosm = "<a href=\"reports/newnodes_$kommunenummer.osm\">missing.osm</a>";
        }
        elseif ($completeness > 20){
            $ledhtml = "<img src=\"yellowled.png\" />";
        }
        elseif ($completeness <= 20){
            $ledhtml = "<img src=\"redled.png\" />";
        }
        $completeness = sprintf("%2.2f %%",$completeness);
        if($new == 0) {
            $completeness = "Ingen data fra Kartverket";
            $ledhtml = "<img src=\"greyled.png\" />";
        }
        if($veivegfixes != ""){
            $veivegfixes = $veivegfixes ." (<a href=\"reports/veivegfixes_$kommunenummer.osm\">osm</a>)";
        }
        if($otherthings != ""){
            $otherthings = $otherthings ." (<a href=\"reports/otherobjects_$kommunenummer.osm\">osm</a>)";
        }
        if($duplicates != ""){
            $duplicates = $duplicates ." (<a href=\"reports/duplicates_$kommunenummer.osm\">osm</a>)";
        }
        $path = sprintf("reports/report2_%04d.txt",$i);
        $fixes = ""; 
        $errors = ""; 
        $onlynumber = 0;
        if(file_exists($path)){
            $file = fopen($path,"r");
            $content = fread($file,10000);
            preg_match("/Fixes:\s+(\d+)\s+Errors:\s+(\d+)/",$content,$matches);
            $fixes = $matches[1];
            $errors = $matches[2];
            if($fixes != 0){
                $fixes = $fixes ." (<a href=\"reports/postcodecityfixes_$kommunenummer.osm\">osm</a>)";
            }
            $ret = preg_match("/Onlynumber:\s+(\d+)/",$content,$matches);
            if($ret == 1)
                $onlynumber = $matches[1];
        }
        $mtime = filemtime('reports/report_'.$kommunenummer.'.txt');
        $nowtime = time();
        $difference = $nowtime - $mtime;
        $modtime = date ("d. M Y H:i:s.",$mtime);
        if ($difference < 172800)
            $modtime = "<font color=\"green\">$modtime</font>";
        echo "<tr><td>$kommunenummer</td><td>".$nrtonavn[$i]."</td><td>$existing</td><td>$buildings</td><td>$new</td><td>$missing</td><td>$notmatched</td><td>$notmatchedpois</td><td align=right>$completeness</td><td>$ledhtml</td><td>$linktoosm</td><td align=right>$veivegfixes</td><td align=right>$otherthings</td><td align=right>$duplicates</td><td align=right>$fixes</td><td align=right>$errors</td><td align=right>$onlynumber</td><td style=\"padding-left : 20px\">$modtime</td></tr>";
    }
}
echo "</table></body></html>";
fclose($file);
?>
