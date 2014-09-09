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
	$year = '2014';
	$host_url = "http://events.unimelb.edu.au/api/v1/events/all/tagged/Faculty%20of%20Arts.xml?auth_token=dsv5n24uLUqtSyZ5Darq&filter=year&year=". $year. "&full=true";

	return $host_url;
}


function _foa_event_dl_xml() {
  global $foa_event_dir;
  $orig_xml_file = $foa_event_dir. '/foa_event_orig.xml';

	$host_url = _foa_event_host_url();

  $ch = curl_init();
  $source = $host_url;
  curl_setopt($ch, CURLOPT_URL, $source);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  $data = curl_exec($ch);
  curl_close($ch);

  $destination = $orig_xml_file;
	_foa_event_write_to_file($data, $destination); 
}


function _foa_event_write_to_file($data, $destination) {
  $file = fopen($destination, "w+");
  fputs($file, $data);
  fclose($file);
}


// http://stackoverflow.com/questions/1193528/how-to-modify-xml-file-using-php
function _foa_event_handle_xml() {
  global $foa_event_dir;
	$orig_xml_file = $foa_event_dir. '/foa_event_orig.xml';

	$dom = new DOMDocument();
	$dom->load($orig_xml_file);

	// This can differ (I am not sure, it can be only documentElement or documentElement->firstChild or only firstChild)
	$root = $dom->documentElement; 
	$events = $root->getElementsByTagName('api-v1-entities-event-item');

	// Build
	$xml = new SimpleXMLElement("<?xml version=\"1.0\" encoding=\"utf-8\" ?><api-v1-entities-event-items></api-v1-entities-event-items>");

	// Loop trough childNodes
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

		/*
		// Location
		// Address
    $child = $item->addChild('location-address');
    $child[0] = $e_location_address;

		// Building
		$child = $item->addChild('location-building');
    $child[0] = $e_location_building;

		// Room-or-theatre
    $child = $item->addChild('location-space');
    $child[0] = $e_location_space;
		*/
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

	// Write to a file
	$data = $xml->saveXML();
	$destination = $foa_event_dir. '/foa_event_mod.xml'; 
  _foa_event_write_to_file($data, $destination);


}


// Main
function _foa_event_modify_xml() {
	_foa_event_dl_xml();
	_foa_event_handle_xml();	
}

