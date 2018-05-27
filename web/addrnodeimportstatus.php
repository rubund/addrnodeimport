<?php
include "mysql.php";
include "kommunenummer.php";
$conn = mysql_connect($dbhost,$dbuser,$dbpass) or die ('Error connecting to mysql');
mysql_select_db($dbname);
mysql_query("set names utf8");
$pending_update_message = "Venter litt før<br>oppdatering (~6 min)";
$pending_update_message2 = "Vil bli oppdatert<br>straks (~1 min)";

header('Content-Type: text/html; charset=utf-8');
echo "<html><head><title>Adressenode-import-status for Norge</title>";
?>
<style>
p {font-family : "Arial";}
td {font-family : "Arial"; font-size : 11pt}
th {font-family : "Arial";}
h3 {font-family : "Arial";}
</style>
<script type="text/javascript">
function ajaxreq(request){
        var xmlhttp;      
        if (window.XMLHttpRequest)      {
            xmlhttp = new XMLHttpRequest();   
        }
        else {
            xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");    
        }
        var url = request;
        xmlhttp.open("GET",url,false);
        xmlhttp.send();  
        return xmlhttp.responseText;
}
var nr2=0;
var kommunenummer=0;
function displayaddname(event,nr){
    var popup_block = document.getElementById('leggtilnavnfelt');
    if(nr2==0){
        var xpos = event.clientX + (document.documentElement.scrollLeft ? document.documentElement.scrollLeft : document.body.scrollLeft);
        var ypos = event.clientY + (document.documentElement.scrollTop ? document.documentElement.scrollTop : document.body.scrollTop);
        popup_block.style.display = 'inline';
        popup_block.style.left = xpos-220+"px";
        popup_block.style.top = ypos-100+"px";
        kommunenummer = nr;
        nr2 = 1;
    }
    else if(nr2==1){
        popup_block.style.display = 'none';
        nr2 = 0;
    }
}
function proposename(){
    var popup_block = document.getElementById('leggtilnavnfelt');
    popup_block.style.display = 'none';
    nr2 = 0;
    var newname = document.getElementById('nyttnavn').value;
    //document.getElementById('ansvarlig_101').innerHTML = newname+" "+kommunenummer;
    var randres;
    if(newname != "")
        randres = ajaxreq("/ajaxpublic.php?person="+newname+"&kommune="+kommunenummer+"");
    else randres = "FEIL";
    document.getElementById('ansvarlig_'+kommunenummer+'').innerHTML = "<b>"+newname+"</b>";
    if (randres == "OK"){
        document.getElementById('ansvarlig_'+kommunenummer+'').innerHTML = "<b>"+newname+"</b>";
    }
    else 
        document.getElementById('ansvarlig_'+kommunenummer+'').innerHTML = "<b>Wasn't able to update</b>";
}
function request_update(kommunenummer_request, step){
    var randres;
    if(step == 1){
	randres = ajaxreq("/ajaxpublic.php?update=1&kommune="+kommunenummer_request+"&now=1");
        
        if (randres == "OK"){
            document.getElementById('oppdater_'+kommunenummer_request+'').innerHTML = "<b><?=$pending_update_message2?></b>";
        }
        else 
            document.getElementById('oppdater_'+kommunenummer_request+'').innerHTML = "<b>Feil</b>";
    }
    else {
	randres = ajaxreq("/ajaxpublic.php?update=1&kommune="+kommunenummer_request+"&now=0");
        if (randres == "OK"){
            document.getElementById('oppdater_'+kommunenummer_request+'').innerHTML = "<input type=\"button\" onclick=\"request_update('"+kommunenummer_request+"',1)\" value=\"Start umiddelbart\"><br><b><?=$pending_update_message?></b>";
        }
        else 
            document.getElementById('oppdater_'+kommunenummer_request+'').innerHTML = "<b>Feil "+randres+"</b>";
    }
}
</script>
<?php
echo "</head>";
echo "<body>";
?>
<div id="leggtilnavnfelt" style="display : none ; position : absolute ; z-index : 99; height : 72px ; width : 520px ; background-color : white ; border : 1px solid black "><p style="margin : 0px ; padding : 0px ; font-size : 10px">Skriv navnet ditt eller nick:</p><input type="text" id="nyttnavn" size="30" /><input type="button" onclick="proposename()" value="Melder meg frivillig!" /> <input type="button" value="Lukk" onclick="displayaddname(event,0)"/></p></div>
<?php
echo "<h3>Adressenode-import-status for Norge</h3>";
//echo "<p>Sist oppdatert: <font color=\"green\">".date ("d. M Y H:i:s.", filemtime('reports/report_1943.txt'))."</font></p>";
echo "<h4>Datauttaksdato: 2018-05-27 (Bruk som <i>source:date</i> i changeset)</h4>";
echo "<pre>source:date=2018-05-27\n";
echo "source=Kartverket</pre>";
?>
<h4>Husk 5 ting:</h4>
<ol>
<li>Opprett en bruker med navnet <i>vanligbrukernavn</i>_import og bruk denne for import</li>
<li>Tag <b>"changeset"</b>-et med source=Kartverket og source:date=dato og <b>ikke</b> nodene.</li>
<li>Ikke importer noder for kommuner f&oslash;r det st&aring;r "<b><font color=\"green\">Klar for å importeres:</font></b>".</li>
<li>Etter innlasting av osm-filer fra denne siden i JOSM, trykk Ctrl-U s&aring;nn at nodene som wayene best&aring;r av lastes inn. Disse nodene er nemlig ikke med i osm-filene!</li>
<li>I enkelte tilfeller kan adressene i OSM være mer oppdatert enn i Kartverkets datasett. Påse at dette ikke ødelegges.</li>
</ol>
<!--<img src="nodes.png" height=700/><img src="nodes-soer.png" height=700/><img src="ways.png" height=700/>-->
<br><br>
<?php
echo "<table border=1>";
echo "<tr><th>&nbsp;</th><th>Kommunenr.</th><th>Kommune</th><th>Fullført</th><th>Status</th><th>Oppdatert</th><th>Ansvarlig person</th><th>For auto</th><th align=\"left\" style=\"padding-left : 30px\">Status / Hva er neste steg?</th></tr>";
for($i=0;$i<2100;$i++){
    $kommunenummer = sprintf("%04d",$i);
    $path = sprintf("reports/report_%04d.txt",$i);
    //echo $path;
    if(file_exists($path)){
        $file = fopen($path,"r");
        $content = fread($file,10000);
        fclose($file);
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
        $completeness = ($new-$missing)*1.0 / $new * 100.0;
        $linktoosm = "";
        if ($missing != 0)
            $linktoosm = "<a href=\"reports/newnodes_$kommunenummer.osm\">newnodes.osm</a>";
        if ($missing == 0){
            $ledhtml = "<img src=\"greenled.png\" />";
        }
        elseif ($completeness > 99){
            $ledhtml = "<img src=\"yellowled.png\" />";
        }
        else{
            $ledhtml = "<img src=\"redled.png\" />";
        }
        $completeness = sprintf("%2.2f %%",$completeness);
        if($new == 0) {
            $completeness = "Ingen data fra Kartverket";
            $ledhtml = "<img src=\"greyled.png\" />";
        }
        if($veivegfixes != ""){
            $veivegfixesstr = $veivegfixes ." (<a href=\"reports/veivegfixes_$kommunenummer.osm\">osm</a>)";
        }
        if($otherthings != ""){
            $otherthingsstr = $otherthings ." (<a href=\"reports/otherobjects_$kommunenummer.osm\">osm</a>)";
        }
        if($duplicates != ""){
            $duplicatesstr = $duplicates ." (<a href=\"reports/duplicates_$kommunenummer.osm\">osm</a>)";
        }
        $path = sprintf("reports/report3_%04d.txt",$i);
        $beforeafter = "";
        if(file_exists($path)){
            $file = fopen($path,"r");
            $content = fread($file,10000);
            fclose($file);
            preg_match("/Before:\s+(\d+)\s+After:\s+(\d+)/",$content,$matches);
            $beforeafter = $content;
            $beforenodes = $matches[1];
            $afternodes = $matches[2];
        }
        else {
            $beforenodes = "";
            $afternodes = "";
        }

        $path = sprintf("reports/report4_%04d.txt",$i);
        if(file_exists($path)){
            $file = fopen($path,"r");
            $content = fread($file,10000);
            fclose($file);
            preg_match("/Changed:\s+(\d+)/",$content,$matches);
            $automaticchanges = $matches[1];
        }
        else {
            $automaticchanges = "";
        }

        $path = sprintf("reports/report2_%04d.txt",$i);
        $fixesstr = ""; 
        $errors = ""; 
        $onlynumber = 0;
        if(file_exists($path)){
            $file = fopen($path,"r");
            $content = fread($file,10000);
            fclose($file);
            preg_match("/Fixes:\s+(\d+)\s+Errors:\s+(\d+)/",$content,$matches);
            $fixes = $matches[1];
            $errors = $matches[2];
            if($fixes != 0){
                $fixesstr = $fixes ." (<a href=\"reports/postcodecityfixes_$kommunenummer.osm\">osm</a>)";
            }
            $ret = preg_match("/Onlynumber:\s+(\d+)/",$content,$matches);
            if($ret == 1)
                $onlynumber = $matches[1];
        }
        //$mtime = filemtime('reports/report_'.$kommunenummer.'.txt');
        $mtime = 0;
        $nowtime = time();
        $result = mysql_query("select updated,unix_timestamp(updated) from municipalities where muni_id='".$i."' limit 1") or die('Mysql error');
        if ($row = mysql_fetch_array($result, MYSQL_NUM)){
            $mtime = $row[1];
        }

        $difference = $nowtime - $mtime;
        $modtime = date ("d. M Y H:i:s.",$mtime);
        if ($difference < 3600)
            $modtime = "<font color=\"green\">$modtime</font>";
        #echo "<tr><td>$kommunenummer</td><td>".$nrtonavn[$i]."</td><td>$existing</td><td>$buildings</td><td>$new</td><td>$missing</td><td>$notmatched</td><td align=right>$completeness</td><td>$ledhtml</td><td>$linktoosm</td><td align=right>$veivegfixesstr</td><td align=right>$otherthings</td><td align=right>$duplicatesstr</td><td align=right>$fixesstr</td><td align=right>$errors</td><td align=right>$onlynumber</td><td style=\"padding-left : 20px\">$modtime</td></tr>";
        $totalstatus = "";
        //$totalstatus .= "$notmatched $fixes $onlynumber $new $missing";
        if($beforenodes > ($afternodes+10)) {
            $totalstatus .= "Klarer ikke definere arealet til kommunen. Tallene stemmer derfor ikke. $afternodes/$beforenodes";
        }
        else if($notmatched == 0 && $fixes == 0 && $onlynumber == 0 && $new != 0 && $missing != 0) {
            $totalstatus .= "<b><font color=\"green\">Klar for å importeres: ".$linktoosm."</font></b> $missing av totalt $new noder i Kartverkets data";
        }
        else if($onlynumber != 0){
            $totalstatus .= "<font color=\"red\">Det finnes adressenoder som mangler gatenavn ($onlynumber stk). Dette må fikses manuelt. Se:</font> <a href=\"reports/onlynumber_$kommunenummer.osm\">onlynumber.osm</a> - <span style=\"color:grey\">[$linktoosm]</span>";
        }
        //else if($fixes != 0){
        //    $totalstatus .= "Det finnes adressenoder som mangler postnr. eller sted ($fixes stk). Denne .osm filen fikser dette: <a href=\"reports/postcodecityfixes_$kommunenummer.osm\">postcodecityfixes.osm</a>";
        //}
        else if($notmatched == 0 && $missing == 0 && $duplicates == 0 && $new != 0) {
            if($otherthings == 0){
                $totalstatus .= "<b><font color=\"green\">Ingen feil!</font></b>";
            }
            else  {
                $totalstatus .= "<b><font color=\"green\">Ingen feil!</font></b> Men det kan være lurt å ta en titt på og reviewe andre objekter ($otherthings stk) som inneholder addr:-tagger: <a href=\"reports/otherobjects_$kommunenummer.osm\">otherobjects.osm</a>";
            }
            //$totalstatus .= "<b><font color=\"green\">Ingen feil!</font> $existing av totalt $new stemmer overens</b>";
        }
        else if($veivegfixes != 0){
            $totalstatus .= "Det finnes adressenoder der vei skulle vært veg eller motsatt ($veivegfixes stk). Denne .osm filen fikser dette: <a href=\"reports/veivegfixes_$kommunenummer.osm\">veivegfixes.osm</a>";
        }
        else if($duplicates != 0){
            $totalstatus .= "<font color=\"red\">Det finnes duplikater ($duplicates stk). Se: <a href=\"reports/duplicates_$kommunenummer.osm\">duplicates.osm</a></font>";
        }
		//else if($notmatched != 0 && $missing != 0 && $automaticchanges != 0){
        //    $totalstatus .= "<strike>Noen eksisterende adresser har endret seg ($automaticchanges stk). Denne .osm filen fikser dette: <a href=\"reports/changed_$kommunenummer.osm\">changed.osm</a></strike>";
		//}
        else if(file_exists("reports/new/$kommunenummer/newnodes_manual_$kommunenummer.osm") && $notmatched != 0 && $missing != 0){
            $totalstatus .= "For manuelt arbeid se: <a href=\"reports/new/$kommunenummer/notmatched_manual_$kommunenummer.osm\">notmatched_manual.osm</a> - sammenlign med <a href=\"reports/new/$kommunenummer/newnodes_manual_$kommunenummer.osm\">newnodes_manual.osm</a>";
        }
        else if($notmatched != 0 && $missing != 0){
            $totalstatus .= "<strike>Noen eksisterende noder gjenkjennes ikke ($notmatched stk). Dette må fikses før import. Se: <a href=\"reports/notmatched_$kommunenummer.osm\">notmatched.osm</a> - sammenlign med $linktoosm</strike>";
        }
        else if($notmatched != 0){
            $totalstatus .= "<strike><font color=\"green\">Noen adressenoder i OSM gjenkjennes ikke ($notmatched stk). Det kan hende dette er uproblematisk. Ellers er alle Kartverkets adressenoder i OSM. Se: <a href=\"reports/notmatched_$kommunenummer.osm\">notmatched.osm</a></font></strike>";
        }
        $result = mysql_query("select person,date(tid) from osmimportresp where kommunenummer='".$i."' and deleted != 1 order by tid asc limit 1") or die('Mysql error');
        if ($row = mysql_fetch_array($result, MYSQL_NUM)){
            $ansvarlig = "<b>".$row[0]."</b> (".$row[1].")";
        }
        else {
            $ansvarlig = "<span onclick=\"displayaddname(event,'$i')\" style=\"text-decoration: underline ; color:grey\">(Melder meg frivillig)</span>";
        }
        $result = mysql_query("select tid from update_requests where kommunenummer='".$i."' and ferdig != 1 order by tid asc limit 1") or die('Mysql error');
        if ($row = mysql_fetch_array($result, MYSQL_NUM)){
            $pending_update = "Vil bli oppdatert<br>straks";
        }
        else {
            $pending_update = "<input type=\"button\" onclick=\"request_update('".$i."',0)\" value=\"Oppdater status\"/>";
        }
        $path = sprintf("reports/report5_%04d.txt",$i);
        if(file_exists($path)){
			$linktoreport  = "<a href=\"reports/report5_$kommunenummer.txt\">report.txt</a>";
			$linktochanges = "<a href=\"reports/new/$kommunenummer/changes_$kommunenummer.osm\">changes.osm</a>";
		}
		else {
			$linktoreport  = "";
			$linktochanges = "";
		}
        
        echo "<tr><td><span id=\"oppdater_$i\">$pending_update</span></td><td>$kommunenummer</td><td>".$nrtonavn[$i]."</td><td align=right>$completeness</td><td>$ledhtml</td><td style=\"padding-left : 20px\">$modtime</td><td style=\"padding-left : 30px\"><span id=\"ansvarlig_$i\">$ansvarlig</span></td><td>$linktoreport<br>$linktochanges</td><td style=\"padding-left : 30px\">$totalstatus</td></tr>";
    }
}
echo "</table></body></html>";
fclose($file);
?>
