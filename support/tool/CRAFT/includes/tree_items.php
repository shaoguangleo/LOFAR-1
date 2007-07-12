<?php

	session_start();

  include("type_object.php");
  $Types_Objecten = array();

	//functie om alle externe contacten uit de database te lezen en deze hierarchisch op te slaan 
  function Locaties_Lijst() {
  	$Collectie = array();
  	$query = 'SELECT Locatie_ID, Loc_Naam FROM comp_locatie';
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Locatie_ID'],$huidige_level['Loc_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	//if ($num_rows > 0) $Comp_Type->Add(Contacten_Lijst($huidige_level['Contact_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }


	//functie om alle gebruikers uit de database te lezen en deze hierarchisch (onder hun groep) op te slaan 
  function Gebruikersgroepen_Lijst($parent) {
  	$Collectie = array();
  	$query = 'SELECT Groep_ID, Groeps_Naam FROM gebruikers_groepen WHERE Groep_Parent = '.$parent;
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Groep_ID'],$huidige_level['Groeps_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	if ($num_rows > 0) $Comp_Type->Add(Gebruikersgroepen_Lijst($huidige_level['Groep_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }
  

	//functie om alle gebruikers uit de database te lezen en deze hierarchisch (onder hun groep) op te slaan 
  function Gebruikers_Lijst() {
  	$Collectie = array();
  	$query = 'SELECT Werknem_ID, inlognaam FROM gebruiker';
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Werknem_ID'],$huidige_level['inlognaam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	//if ($num_rows > 0) $Comp_Type->Add(Contacten_Lijst($huidige_level['Contact_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }


	//functie om alle externe contacten uit de database te lezen en deze hierarchisch op te slaan 
  function Contacten_Lijst($parent) {
  	$Collectie = array();
  	$query = 'SELECT Contact_ID, Contact_Naam FROM contact WHERE Contact_Parent = '.$parent;
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Contact_ID'],$huidige_level['Contact_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	if ($num_rows > 0) $Comp_Type->Add(Contacten_Lijst($huidige_level['Contact_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }


	//functie om alle type meldingen uit de database te lezen en deze hierarchisch op te slaan 
  function Melding_Type_Lijst() {
  	$Collectie = array();
  	$query = 'SELECT Meld_Type_ID, Melding_Type_Naam FROM melding_type';
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Meld_Type_ID'],$huidige_level['Melding_Type_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	//if ($num_rows > 0) $Comp_Type->Add(Comp_Lijst($huidige_level['Meld_Type_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }
  
  
  //functie om alle componenten uit de database te lezen en deze hierarchisch op te slaan
  function Comp_Lijst($parent) {
  	$Collectie = array();
  	$query = 'SELECT a.Comp_Lijst_ID, a.Comp_Naam FROM comp_lijst a, comp_type b WHERE a.Comp_Type_ID = b.Comp_Type AND a.Comp_Parent = '.$parent;
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Comp_Lijst_ID'],$huidige_level['Comp_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	if ($num_rows > 0) $Comp_Type->Add(Comp_Lijst($huidige_level['Comp_Lijst_ID']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
	  }
  	return $Collectie;	
  }
  
	//functie om alle type componenten uit de database te lezen en deze hierarchisch op te slaan 
  function Comp_Type_Lijst($parent) {
  	$Collectie = array();
  	$query = 'SELECT Comp_Type, Type_Naam FROM comp_type WHERE Type_Parent = '.$parent;
	  $resultaat = mysql_query($query);
  	while ($huidige_level = mysql_fetch_array($resultaat)) {
  		$Comp_Type = new Type_Object();
  		$Comp_Type->Set_ID($huidige_level['Comp_Type'],$huidige_level['Type_Naam']);
	  	$num_rows = mysql_num_rows(mysql_query($query));		
	  	if ($num_rows > 0) $Comp_Type->Add(Comp_Type_Lijst($huidige_level['Comp_Type']));
 	  	array_push($Collectie, $Comp_Type);
	  	$Comp_Type = NULL;
  	}  
  	return $Collectie;	
  }
  
  function Recursieve_Uitlees_Methode($type_object) {
  	$uitkomst = '';
  	for ($i = 0; $i < count($type_object);$i++) {
			$uitkomst = $uitkomst . "['". $type_object[$i]->Get_Naam()."', '". $_SESSION['huidige_pagina']. "&c=". $type_object[$i]->Get_ID() ."'";
  		$temp = $type_object[$i]->Get_Childarray();
			if (count($temp) > 0) $uitkomst = $uitkomst . ", ";
  		$uitkomst = $uitkomst . Recursieve_Uitlees_Methode($temp);
    	$uitkomst = $uitkomst . "],";
  	}
  	return $uitkomst;
  }
  
  
  $link = mysql_connect("localhost", "root", "root") or die("Kan niet verbinden: " . mysql_error());
  mysql_select_db('LOFAR-CRAFT', $link) or die('Could not select database.');
  $query = 'SELECT Comp_Type, Type_Naam FROM comp_type WHERE Type_Parent = 1';
  $rest = mysql_query($query);
  
  if ($_SESSION['admin_deel'] == 1)      $Types_Objecten = Comp_Type_Lijst(1);
  else if ($_SESSION['admin_deel'] == 2) $Types_Objecten = Comp_Lijst(1);
  else if ($_SESSION['admin_deel'] == 3) $Types_Objecten = Melding_Type_Lijst();
	else if ($_SESSION['admin_deel'] == 4) $Types_Objecten = Comp_Lijst(1);
	else if ($_SESSION['admin_deel'] == 5) $Types_Objecten = Gebruikersgroepen_Lijst(1);
	else if ($_SESSION['admin_deel'] == 6) $Types_Objecten = Gebruikers_Lijst();
	else if ($_SESSION['admin_deel'] == 7) $Types_Objecten = Contacten_Lijst(1);
	else if ($_SESSION['admin_deel'] == 8) $Types_Objecten = Locaties_Lijst();
  else if ($_SESSION['admin_deel'] == 0) {
  	if ($_SESSION['main_deel'] == 2) {
  		if ($_SESSION['type_overzicht'] == 1) 		 $Types_Objecten = Comp_Lijst(1);
  		else if ($_SESSION['type_overzicht'] == 2) $Types_Objecten = Comp_Type_Lijst(1);
  	}
  }
	echo ("var TREE_ITEMS = [");
	
	echo(Recursieve_Uitlees_Methode($Types_Objecten));
	
	echo ("];");

  mysql_close($link);  

?>