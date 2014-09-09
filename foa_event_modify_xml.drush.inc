<?php

$config_path = dirname(__FILE__). "/config/foa_event.config.php";
include_once($config_path);

/**
 * Implements hook_drush_command().
 */
function foa_event_modify_xml_drush_command() {
  $items['foa_event_modify_xml'] = array(
    'callback' => '_foa_event_modify_xml',
    'description' => dt('foa_event_modify_xml'),
    'examples' => array(
      'simple' => dt('drush foa_event_modify_xml'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL,
  );
  return $items;
}


// Change it here
function _foa_event_host_url() {
	$host_url = "http://events.unimelb.edu.au/api/v1/events/all/tagged/Faculty%20of%20Arts.xml?auth_token=dsv5n24uLUqtSyZ5Darq&filter=month&month={month}&year={year}&full=true";

	return $host_url;
}


function _foa_event_dl_xml() {
	global $foa_event_dir;

	// Clean up
	rrmdir($foa_event_dir. "/orig/files");
	mkdir($foa_event_dir. "/orig/files");

	$filter_array = _foa_event_build_filter();
	foreach($filter_array as $item) {
		$month = $item['month'];
		$year = $item['year'];
		$url = $item['url'];
	
		// fwrite won't create dir	
		$orig_xml_file = $foa_event_dir. "/orig/files/". $month. "_". $year. '.xml';
		
		$ch = curl_init();
  	$source = $url;
  	curl_setopt($ch, CURLOPT_URL, $source);
  	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  	$data = curl_exec($ch);
  	curl_close($ch);

  	$destination = $orig_xml_file;
  	_foa_event_write_to_file($data, $destination);	
	}	
}


// http://stackoverflow.com/questions/3338123/how-do-i-recursively-delete-a-directory-and-its-entire-contents-files-sub-dir
function rrmdir($dir) { 
	if(is_dir($dir)) { 
  	$objects = scandir($dir); 
    foreach($objects as $object) { 
    	if($object != "." && $object != "..") { 
      	if(filetype($dir."/".$object) == "dir") rrmdir($dir."/".$object); else unlink($dir."/".$object); 
      } 
    } 
    reset($objects); 
    rmdir($dir); 
  } 
}

function _foa_event_build_filter() {
  $curr_month = date('n');
  $curr_year = date('Y');

  $filter_array = _foa_event_cal_month($curr_month, $curr_year);
  return $filter_array;
}


function _foa_event_cal_month($curr_month, $curr_year) {
  $out_1 = _foa_event_past_month($curr_month, $curr_year);
  $out_2 = _foa_event_curr_month($curr_month, $curr_year);
  $out_3 = _foa_event_future_month($curr_month, $curr_year);

  $out = array_merge($out_1, $out_2, $out_3);

  //test
  //print_r($out);

  $filter_array = _foa_event_build_filter_url($out);
  return $filter_array;
}


function _foa_event_past_month($curr_month, $curr_year) {
  // Past 2 months
  $len = 2;
  $total_month = 12;
  $past_month = 0;
  $past_year = 0;
  $output = array();

  for($i=1; $i<=$len; $i++) {
    $tmp_month = $curr_month - $i;
    if($tmp_month <= 0) {
      $past_month = $tmp_month + $total_month;
      $past_year = $curr_year - 1;
    }
    else {
      $past_month = $tmp_month;
      $past_year = $curr_year;
    }

    // The key won't overlap given the current range
    $key = $past_month;
    $output[$key] = array(
      'month' => $past_month,
      'year' => $past_year,
    );
  }
  ksort($output);

  //test
  //print_r($output);

  return $output;
}


function _foa_event_curr_month($curr_month, $curr_year) {
  $output = array();
  $key = $curr_month;
  $output[$key] = array(
    'month' => $curr_month,
    'year' => $curr_year,
  );

  //test
  //print_r($output);

  return $output;
}


function _foa_event_future_month($curr_month, $curr_year) {
  // Future 6 months
  $len = 6;
  $total_month = 12;
  $f_month = 0;
  $f_year = 0;
  $output = array();

  for($i=1; $i<=$len; $i++) {
    $tmp_month = $curr_month + $i;
    $diff = $tmp_month - $total_month;
    if($diff <= 0) {
      $f_month = $tmp_month;
      $f_year = $curr_year;
    }
    else {
      $f_month = $tmp_month - $total_month;
      $f_year = $curr_year + 1;
    }

    $key = $f_month;
    $output[$key] = array(
      'month' => $f_month,
      'year' => $f_year,
    );
  }

  //test
  //print_r($output);

  return $output;
}


function _foa_event_build_filter_url($input) {
  $copy_host_url = _foa_event_host_url();
  $filter_url = '';
  $output = array();

  foreach($input as $item) {
    $month = $item['month'];
    $year = $item['year'];

    $vars = array(
      '{month}' => $month,
      '{year}' => $year,
    );

    $filter_url = strtr($copy_host_url, $vars);
    $output[] = array(
      'month' => $month,
      'year' => $year,
      'url' => $filter_url,
    );
  }

  //test
  //print_r($output);

  return $output;
}


function _foa_event_write_to_file($data, $destination, $option="w+") {
  $file = fopen($destination, $option);
  fputs($file, $data);
  fclose($file);
}


function _foa_event_handle_xmls() {
	global $foa_event_dir;
	$orig_xml_dir = $foa_event_dir. "/orig/files";

	// Build
  $xml = new SimpleXMLElement("<?xml version=\"1.0\" encoding=\"utf-8\" ?><api-v1-entities-event-items></api-v1-entities-event-items>");

	if($handle = opendir($orig_xml_dir)) {
  	while(false !== ($entry = readdir($handle))) {
    	if($entry != "." && $entry != "..") {
				$input_file = $orig_xml_dir. "/". $entry;
				_foa_event_handle_xml($xml, $input_file); 
      }
    }
    closedir($handle);
	}

	// Write to a file
  $data = $xml->saveXML();
  $destination = $foa_event_dir. '/mod/files/foa_event.xml';
  _foa_event_write_to_file($data, $destination, 'w+');		
}


// For new line needs to use "\n", not '\n'.
function _foa_event_handle_xml(&$xml, $input_file) {
	$dom = new DOMDocument();
  $dom->load($input_file);

	// This can differ (I am not sure, it can be only documentElement or documentElement->firstChild or only firstChild)
  $root = $dom->documentElement; 
  $events = $root->getElementsByTagName('api-v1-entities-event-item');

	// Loop through childNodes
	foreach($events as $event) {
		$e_id = $event->getElementsByTagName('id')->item(0)->textContent;
    $e_title = $event->getElementsByTagName('title')->item(0)->textContent;
    $e_type = $event->getElementsByTagName('event-type')->item(0)->textContent;
    $e_live_stream = $event->getElementsByTagName('has-livestream')->item(0)->textContent;

    $e_host = $event->getElementsByTagName('host')->item(0)->textContent; 
    $e_start_time = $event->getElementsByTagName('start-time')->item(0)->textContent; 
    $e_public = $event->getElementsByTagName('public')->item(0)->textContent;
    $e_end_time = $event->getElementsByTagName('end-time')->item(0)->textContent;

    $e_sold_out = $event->getElementsByTagName('sold-out')->item(0)->textContent; 
    $e_cancel = $event->getElementsByTagName('cancelled')->item(0)->textContent;
    $e_desc_html = $event->getElementsByTagName('description-html')->item(0)->textContent;
  
    // Key: get by tag name, return array, so use item(0)
    $e_location_address = $event->getElementsByTagName('location')->item(0)->getElementsByTagName('address')->item(0)->textContent; 
    $e_location_building = $event->getElementsByTagName('location')->item(0)->getElementsByTagName('building')->item(0)->textContent; 
    $e_location_space = $event->getElementsByTagName('location')->item(0)->getElementsByTagName('room-or-theatre')->item(0)->textContent;


		// Merge to location
    $tmp_array = array(
      $e_location_space,
      $e_location_building,
      $e_location_address,
    );
    $tmp_array = array_filter($tmp_array);
    $e_location = implode(', ', $tmp_array);
    
    $e_info_email = $event->getElementsByTagName('information')->item(0)->getElementsByTagName('email')->item(0)->textContent;
    $e_info_phone = $event->getElementsByTagName('information')->item(0)->getElementsByTagName('phone')->item(0)->textContent;
    $e_info_url = $event->getElementsByTagName('information')->item(0)->getElementsByTagName('url')->item(0)->textContent;

    $e_book_email = $event->getElementsByTagName('booking')->item(0)->getElementsByTagName('email')->item(0)->textContent;
    $e_book_phone = $event->getElementsByTagName('booking')->item(0)->getElementsByTagName('phone')->item(0)->textContent;    
    $e_book_url = $event->getElementsByTagName('booking')->item(0)->getElementsByTagName('url')->item(0)->textContent;

    $e_link = $event->getElementsByTagName('link')->item(0)->textContent;


		// Presenters
    $p_objs = $event->getElementsByTagName('presenter');
    $p_name_total_html = '';
    $p_link_total_html = '';
    for($i=0; $i < $p_objs->length; $i++) {
      $p_title = trim( $p_objs->item($i)->getElementsByTagName('title')->item(0)->textContent );
      $p_first_name = trim( $p_objs->item($i)->getElementsByTagName('first-name')->item(0)->textContent );
      $p_last_name = trim( $p_objs->item($i)->getElementsByTagName('last-name')->item(0)->textContent );      
      $p_url = trim( $p_objs->item($i)->getElementsByTagName('link')->item(0)->textContent );

      // $p_title can be empty.

      // Name
      $p_name_html = trim( $p_title. " ". $p_first_name. " ". $p_last_name );
      $p_name_total_html .= $p_name_html. "|";

      // Link
      $p_link_total_html .= $p_url. "|";
    }
    $p_name_total_html = substr($p_name_total_html, 0, -1);
    $p_link_total_html = substr($p_link_total_html, 0, -1);

    
    // Tags
    $tag_objs = $event->getElementsByTagName('tag');      
    $tag_total_html = '';   
    for($i=0; $i < $tag_objs->length; $i++) {
      $tag = $tag_objs->item($i)->getElementsByTagName('name')->item(0)->textContent;
      $tag_total_html .= $tag. "|";
    }
    $tag_total_html = substr($tag_total_html, 0, -1);


		// Start to build 
    $item = $xml->addChild('api-v1-entities-event-item');

    // Event id
    $item->addChild('id', $e_id);
    
    // Event title
    // http://stackoverflow.com/questions/17027043/unterminated-entity-reference-php
    $child = $item->addChild('title');
    $child[0] = $e_title; 

    // Event type
    $child = $item->addChild('event-type');
    $child[0] = $e_type;

    // Live stream
    $child = $item->addChild('has-livestream');
    $child[0] = $e_live_stream; 

    // Host 
    $child = $item->addChild('host');
    $child[0] = $e_host;

    // Start time
    $child = $item->addChild('start-time');
    $child[0] = $e_start_time;

    // Public
    $child = $item->addChild('public');
    $child[0] = $e_public;
  
    // End time
    $child = $item->addChild('end-time');
    $child[0] = $e_end_time;
		
		// Sold out
    $child = $item->addChild('sold-out');
    $child[0] = $e_sold_out;  

    // Cancelled
    $child = $item->addChild('cancelled');
    $child[0] = $e_cancel;

    // Presenter names
    $child = $item->addChild('presenter-name');
    $child[0] = $p_name_total_html;

    // Presenter links
    $child = $item->addChild('presenter-link');
    $child[0] = $p_link_total_html;

    // Event description html
    $child = $item->addChild('description-html');
    $child[0] = $e_desc_html;

    // Location
    $child = $item->addChild('location');
    $child[0] = $e_location;

		// Information
    // Email
    $child = $item->addChild('info-email');
    $child[0] = $e_info_email;

    // Phone
    $child = $item->addChild('info-phone');
    $child[0] = $e_info_phone;

    // URL
    $child = $item->addChild('info-url');
    $child[0] = $e_info_url;

    // Booking
    // Email
    $child = $item->addChild('book-email');
    $child[0] = $e_book_email;

    // Phone
    $child = $item->addChild('book-phone');
    $child[0] = $e_book_phone;

    // URL 
    $child = $item->addChild('book-url');
    $child[0] = $e_book_url;  

    // Event link
    $child = $item->addChild('link');
    $child[0] = $e_link;
  
    // Tag
    $child = $item->addChild('tag');
    $child[0] = $tag_total_html;

	}	
}

// Main
function _foa_event_modify_xml() {
	_foa_event_dl_xml();
	_foa_event_handle_xmls();	
}

