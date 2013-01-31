<?php
/**
 * @file
 * Drush plugin to scrape content from HTML pages.
 */

/**
 * Implements hook_drush_command().
 */
function scrape_drush_command() {
  $items['scrape'] = array(
    'callback' => 'drush_scrape',
    'description' => dt('Scrape content from web pages.'),
    'arguments' => array(
      'list' => dt('File containing a list of URIs to scape.'),
    ),
    'options' => array(
      'source-url' => dt('Host URL to prepend to all URIs in file.'),
      'source-title' => dt('Specify an element selector for the page title.'),
      'source-content' => dt('Specify an element selector for the page content.'),
      'target-node' => dt('Specify the type of node to create on the target Drupal.'),
      'target-files' => dt('Specify the file field on the target nodes that should store document attachments.'),
      'target-images' => dt('Specify the image field on the target nodes that should store images.'),
    ),
    'examples' => array(
      'simple' =>  dt('drush scrape filename.txt'),
      'with url' => dt('drush scrape --url=http://ltrc.unimelb.edu.au filename.txt'),
      'complex' => dt('drush scrape --source-url=http://ltrc.unimelb.edu.au --source-title=title --source-content=div[id=content] --target-node=page --target-files=field_attachment --target-images=field_image filename.txt'),
      'source csv columns' => dt('OldPath,NewPath,OldTitle,NewTitle'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  // Hardcoded equipment import.
  $items['equipment-import'] = array(
    'callback' => 'drush_equipment_import',
    'description' => dt('Import a well-formatted equipment CSV to equipment nodes.'),
    'arguments' => array(
      'csv' => dt('File containing the equipment information.'),
    ),
    'examples' => array(
      'columns' => dt('Type,Name,Description,Manual,ImageURL,BookingURL,ResourceTitle,ResourceURL,Site'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

	// Warning: delete all node of same type
	$items['delete_nodes'] = array( 
		'callback' => 'drush_delete_nodes',
    'description' => dt('Warning: delete all node of same type'),
    'arguments' => array(
      'node_type' => dt('Node type'),
    ),
    'examples' => array(
      'simple' => dt('drush delete_nodes award'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
	);

	// Warning: delete all award nodes, expect the ones defined
	$items['delete_awards'] = array(
		'callback' => 'drush_delete_awards',
    'description' => dt('Warning: delete all award nodes, expect the ones defined'),
    'examples' => array(
      'simple' => dt('drush delete_awards'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
	);

	// Import award in spreadsheet to drupal node
	$items['import_award'] = array(
		'callback' => 'drush_import_award',
    'description' => dt('Import a well-formatted equipment CSV to award nodes.'),
    'arguments' => array(
      'input' => dt('File containing a list of wards'),
    ),
    'examples' => array(
      'simple' => dt('drush import_award /full_path/filename.xlsx'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  $items['link-slurp'] = array(
    'callback' => 'drush_scrape_slurp',
    'description' => dt('Slurp links from a web page.'),
    'arguments' => array(
      'url' => dt('URL to the page from which to slurp the links.'),
    ),
    'options' => array(
      'element' => dt('Element that encluses the links to slurp. Eg: a navigation menu div.'),
      'path-only' => dt('Return only the path, stripping the protocol and hostname.'),
    ),
    'examples' => array(
      'simple' =>  dt('drush link-slurp --element="div[id=menu]" http://www.google.com/'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

/**
 * Command callback.
 */
function drush_scrape($list = '') {
  if (empty($list)) {
    drush_set_error('NO_FILE', dt('I require a file with a list of URLs as argument.'));
    return 1;
  }
  if (!file_exists($list)) {
    drush_set_error('NO_FILE', dt('List file "@file" does not exist.', array('@file'=> $list)));
    return 1;
  }

  require_once('simple_html_dom.inc');

  $source_url = drush_get_option('source-url');

  $title = drush_get_option('source-title');
  if (empty($title)) {
    $title = 'title';
  }
  $content = drush_get_option('source-content');
  if (empty($content)) {
    $content = 'div[id=content]';
  }

  $target_node = drush_get_option('target-node');
  if (empty($target_node)) {
    $target_node = 'page';
  }
  $target_files = drush_get_option('target-files');
  if (empty($target_files)) {
    $target_files = 'field_attachment';
  }
  $target_images = drush_get_option('target-images');
  if (empty($target_images)) {
    $target_images = 'field_image';
  }

  $fp = fopen($list, 'r');
  if (!$fp) {
    drush_set_error('FILE_PERM', dt('Cannot open file @file.', array('@file'=> $list)));
  }

  while (!feof($fp)) {

    $data = fgetcsv($fp, 2048);
    if (!count($data))
      continue;

    $uri = trim($data[0]);
    if (empty($uri))
      continue;

    // Skip .php files.
    $parts = pathinfo($uri);
    if ($parts['extension'] == 'php') {
      continue;
    }

    // The source file can have a few more fields, which control a
    // search & replace of the title and the new URL.
    if (!empty($data[1])) {
      $new_path = trim($data[1]);
    }
    else {
      $new_path = '';
    }
    if (!empty($data[2])) {
      $old_title = trim($data[2]);
    }
    else {
      $old_title = '';
    }
    if (!empty($data[3])) {
      $new_title = trim($data[3]);
    }
    else {
      $new_title = '';
    }

    if (!empty($source_url)) {
      $url = $source_url . $uri;
    }
    else {
      $url = $uri;
    }

    // Grab etc.
    $page = file_get_html($url);
    if (empty($page))
      continue;

    $uri = parse_url($url);

    $page_title = empty($new_title) ? trim($page->find($title, 0)->plaintext) : $new_title;
    $content_title = trim($page->find($content. ' h2', 0)->plaintext);
    $page_body = trim($page->find($content, 0));

    // Remove all HTML comments from the body.
    $page_body = preg_replace('/<!--(.*)-->/Uis', '', $page_body);

    // Remove all superfluous spacing. Though it seems like a silly thing
    // to do, this will help with search & replace later on.
    $page_body = str_replace(array("\r\n", "\r"), "\n", $page_body);
    $lines = explode("\n", trim($page_body));
    $nlines = array();

    foreach ($lines as $line) {
      if(!empty($line)) {
        $nlines[] = trim($line);
      }
    }
    $page_body = implode($nlines);

    // Clean up.
    unset($lines);
    unset($nlines);

    if (empty($page_body)) {
      drush_print(dt('No page content for @url', array('@url' => $url)));
      $page->clear();
      continue;
    }

    $replacements = drush_scrape_get_links($url, $page, $content);
    $page_files = drush_scrape_get_files($url, $page, $content);
    $page_images = drush_scrape_get_images($url, $page, $content);

    // Loop through the page body and add the links to drupal-hosted files.
    $files = array();
    foreach ($page_files as $link => $file) {
      $furl = file_create_url($file['uri']);
      $furi = drupal_parse_url($furl);
      $replacements[$link] = $furi['path'];
      $files[] = $file;
    }

    // Loop through the page body and add the links to drupal-hosted images.
    $images = array();
    foreach ($page_images as $link => $image) {
      $furl = file_create_url($image['uri']);
      $furi = drupal_parse_url($furl);
      $replacements[$link] = $furi['path'];
      $images[] = $image;
    }

    // Pull the in-content h2 page title from the page.
    $replacements['<h2>' . $old_title . '</h2>'] = '';
    $replacements['<H2>' . $old_title . '</H2>'] = '';

    // Pull the in-content h2 page title from the page.
    $replacements['<h2>' . $page_title . '</h2>'] = '';
    $replacements['<H2>' . $page_title . '</H2>'] = '';

    // Pull the first in-content h2 page title from the page regardless.
    $replacements['<h2>' . $content_title . '</h2>'] = '';
    $replacements['<H2>' . $content_title . '</H2>'] = '';

    // Replace ALL THE THINGS!
    $page_body = strtr($page_body, $replacements);

    // New path?
    if (empty($new_path)) {
      $path = _drush_scrape_deindex(substr($uri['path'], 1));
    }
    else {
      $path = substr($new_path, 1);
    }
    $path = rtrim($path, '/');

    $node = new stdClass();
    $node->type = $target_node;
    $node->nid = NULL;
    $node->uid = 1;
    $node->title = check_plain($page_title);
    $node->language = LANGUAGE_NONE;
    $node->body[$node->language] = array(0 => array('format'=> 'full_html', 'value' => $page_body));
    $node->path = array('alias' => $path);
    $node->{$target_files}[$node->language] = $files;
    $node->{$target_images}[$node->language] = $images;

    node_save($node);

    drush_print(dt('Saved @title as @type @nid (@alias)', array(
      '@title' => $page_title,
      '@type' => $target_node,
      '@nid' => $node->nid,
      '@alias' => $node->path['alias'])
    ));

    $page->clear();

    // Do not spam the source server to death.
    sleep(1);
  }
  fclose($fp);
}

/**
 * Helper to extract and download local images.
 */
function drush_scrape_get_images($url, $page, $content) {
  $images = array();
  foreach ($page->find($content . ' img') as $image) {
    // Absolute local image.
    if ($image->src[0] == '/') {
      // Prepend the hostname part of the URL only.
      $uri = parse_url($url);
      // Download, store.
      $file = drush_scrape_get_file($uri['scheme'] . '://' . $uri['host'] . $image->src, $image->alt);
      if (!empty($file))
        $images[$image->src] = $file;
    }
    // Remote link
    else if (strpos($link->href, 'http') !== FALSE) {
      // Do not fetch.
    }
    // Local relative link.
    else {
      // Prepend the dirname() of the current url.
      // Download, store.
      $file = drush_scrape_get_file(dirname($url) . '/' . $image->src, $image->alt);
      if (!empty($file))
        $images[$image->href] = $file;
    }
  }

  return $images;
}

/**
 * Helper to extract and download local files.
 */
function drush_scrape_get_files($url, $page, $content) {
  $files = array();
  foreach ($page->find($content . ' a') as $link) {
    // HTML Link?
    if (strpos($link->href, '.htm') !== FALSE)
      continue;

    // Directory link?
    if (strpos($link->href, '.') === FALSE)
      continue;

    // Absolute local link.
    if ($link->href[0] == '/') {
      // Prepend the hostname part of the URL only.
      $uri = parse_url($url);
      // Download, store.
      $file = drush_scrape_get_file($uri['scheme'] . '://' . $uri['host'] . $link->href, $link->plaintext);
      if (!empty($file))
        $files[$link->href] = $file;
    }
    // Remote link
    else if (strpos($link->href, 'http') !== FALSE) {
      // Do not fetch.
    }
    // Anchor
    else if (strpos($link->href[0], '#') !== FALSE) {
      // Do not fetch.
    }
    // Local relative link.
    else {
      // Prepend the dirname() of the current url.
      // Download, store.
      $file = drush_scrape_get_file(dirname($url) . '/' . $link->href, $link->plaintext);
      if (!empty($file))
        $files[$link->href] = $file;
    }
  }
  return $files;
}

/**
 * Helper that retrieves a file and stores it locally.
 */
function drush_scrape_get_file($url, $title = '') {
  $filename = basename($url);
  $file = system_retrieve_file($url, "public:///" . $filename, TRUE, FILE_EXISTS_REPLACE);
  if ($file) {
    drush_print(dt('Downloaded and saved @file', array('@file' => $file->uri)));
    return array(
      'fid' => $file->fid,
      'display' => 1,
      'description' => $title,
      'uid' => 1,
      'filename' => $file->filename,
      'uri' => $file->uri,
      'filemime' => $file->filemime,
      'filesize' => $file->filesize,
      'status' => 1,
      'timestamp' => $file->timestamp,
      'rdf_mapping' => array(),
    );
  }
}

/**
 * Command callback.
 *
 * Import CSV data into equipment nodes.
 * 'Type,Name,Description,Manual,ImageURL,BookingURL,ResourceTitle,ResourceURL,Site'
 */
function drush_equipment_import($list = '') {
  if (empty($list)) {
    drush_set_error('NO_FILE', dt('I require a CSV file with a list of columns.'));
    return 1;
  }
  if (!file_exists($list)) {
    drush_set_error('NO_FILE', dt('List file "@file" does not exist.', array('@file'=> $list)));
    return 1;
  }

  $fp = fopen($list, 'r');
  if (!$fp) {
    drush_set_error('FILE_PERM', dt('Cannot open file @file.', array('@file'=> $list)));
  }

  $items = array();

  while (!feof($fp)) {
    $data = fgetcsv($fp, 2048);

    // No data, skip.
    if (!count($data))
      continue;

    // Header row, skip.
    if ($data[0] == 'Type')
      continue;

    if (!empty($data[0])) {
      // New item, shunt the old one onto the array processed items.
      if (!empty($item)) {
        $items[] = $item;
      }

      $item = array();
      $item['images'] = array();
      $item['resources'] = array();

      $item['type'] = trim($data[0]);
      $item['title'] = trim($data[1]);
      $item['description'] = trim($data[2]);
      $item['manual'] =  trim($data[3]);
      $item['image'] = trim($data[4]);
      $item['resources'][] = array('title' =>  trim($data[4]), 'url' => trim($data[5]));
    }
    else {
      // Extra resources on a line by themselves.
      $item['resources'][] = array('title' =>  trim($data[4]), 'url' => trim($data[5]));
    }
  }
  fclose($fp);

  // Process the array of items into nodes.
  foreach ($items as $item) {
    $node = new stdClass();
    $node->field_equipment_type[LANGUAGE_NONE][] = array('value' => $item['type']);
    $node->title = $item['title'];
    $node->body[LANGUAGE_NONE][] = array('value' => $item['description']);
    $node->field_equipment_manual[LANGUAGE_NONE][] = array('url' => $item['manual']);

    // Image - fetch, save attach.
    $node->field_equipment_image[LANGUAGE_NONE][] = drush_scrape_get_file($item['image']);

    // Resources.
    foreach ($item['resources'] as $resource) {
      $node->field_equipment_resources[LANGUAGE_NONE][] = array('url' => $resource['url'], 'title' => $resource['title']);
    }

    node_save($node);
  }
}

function drush_import_award($input = "")
{
	// Runtime start
	$runtime_start = getrusage();

	if(empty($input)) 
	{
    drush_set_error('NO_FILE', dt('I require a spreadsheet file with a list of columns.'));
    return 1;
  }
  
  if(!file_exists($input)) 
  {
    drush_set_error('NO_FILE', dt('List file "@file" does not exist.', array('@file'=> $input)));
    return 1;
  }
	
	$phpexcel_path = "lib/phpexcel/Classes";
	require_once $phpexcel_path. "/PHPExcel/IOFactory.php";
	
	// Smaller cell size
	$cacheMethod = PHPExcel_CachedObjectStorageFactory::cache_in_memory;
	PHPExcel_Settings::setCacheStorageMethod($cacheMethod);
	
	$input_file_path = $input;
	$input_file_type = PHPExcel_IOFactory::identify($input_file_path);
	
	$reader = PHPExcel_IOFactory::createReader($input_file_type);
	$excel_obj = $reader->load($input_file_path);
	
	$excel_obj->setActiveSheetIndex(0);
	$worksheet_obj = $excel_obj->getActiveSheet();
	
	$highest_column = $worksheet_obj->getHighestColumn(); // e.g. it outputs AMK
	$highest_row = $worksheet_obj->getHighestRow(); // e.g. it outputs 228
	
	// Don't import row 3 to row 30
	//$row_to_skip = 2;
	$row_to_skip = 30;
	
	for($row = 1; $row <= $highest_row; ++$row) 
	{
		// -------------------------------- Start skip
		// Just skip some rows
		if($row <= $row_to_skip)
			continue;
		
		// Skip no award type
		$col_string = "F";
		$col_range = 3;
		if(_is_no_award_type($worksheet_obj, $col_string, $col_range, $row))	
			continue;
		// -------------------------------- End skip
		
	
		// Award name
		$col_string = "E";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$award_name = $output_array["value"];
		$award_name_link = $output_array["link"];
		
		if(empty($award_name))
			continue;
	
		// Trust
		$col_string = "C";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$trust = $output_array["value"];
		$trust_link = $output_array["link"];
		
		// Skip non-trust
		
		// Award type
		$col_string = "F";
		$col_range = 3;
		$award_type_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
		
		// Skip Faculty level
		
		// Finanicial need
		$col_string = "O";
		$finanicial_need = _award_get_cell_value_bicondition($worksheet_obj, $col_string, $row);

		// Study level
		$col_string = "P";
		$col_range = 6;
		$study_level_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
		
		// Citizenship
		$col_string = "V";
		$col_range = 3;
		$citizenship_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
		$citizenship_array = _flatten_array($citizenship_array);
		
		// Travel support
		$col_string = "Y";
		$travel_support = _award_get_cell_value_bicondition($worksheet_obj, $col_string, $row);
		
		// Term
		$col_string = "Z";
		$term = _award_get_numeric_cell_value($worksheet_obj, $col_string, $row);
		
		// Approximate value
		$col_string = "AA";
		$award_value = _award_get_numeric_cell_value($worksheet_obj, $col_string, $row);
		
		// Area of study
		$col_string = "AB";
		$study_area_array = _award_get_study_area_table_ids($worksheet_obj, $col_string, $row);
		
		// Background
		$col_string = "AC";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$background = $output_array["value"];
		$background = _destill_field_content($background);
		
		// Application process
		$col_string = "AD";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$application_process = $output_array["value"];
		$application_process = _destill_field_content($application_process);
		
		// Conditions
		$col_string = "AE";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$conditions = $output_array["value"];
		$conditions = _destill_field_content($conditions);
		
		// Students (i.e. student type)
		$col_string = "AF";
		$col_range = 3;
		$student_type_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
		
		// Other
		$col_string = "AI";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$other = $output_array["value"];
		
		// Apply link
		$col_string = "AJ";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$apply_link = $output_array["value"];
		$apply_link = _get_valid_url($apply_link);
		
		// Enquiry now email
		$col_string = "AK";
		$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
		$enquiry_email = $output_array["value"];
		
		// Open day
		$col_string = "AL";
		$open_day = _award_get_date_in_cell($worksheet_obj, $col_string, $row);
		
		// Close day
		$col_string = "AM";
		$close_day = _award_get_date_in_cell($worksheet_obj, $col_string, $row);
		
		// Frequency
		$col_string = "AN";
		$col_range = 4;
		$frequency_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
		
		// Site, this indicates which website this award should go to
		$site_term_id = _award_get_default_site("arts");

		
		// Save node
		$node = new stdClass();
    $node->type = "award";
    $node->nid = NULL;
    $node->uid = 1;
    
    $node->language = LANGUAGE_NONE;
    $node->title = $award_name;
    
    // Background
    if($award_name_link != null)
    {
    	$background = l($award_name, $award_name_link). "<br/><br/>". $background;
    }
    
    
    //test
    //echo "\n$award_name_link|". l($award_name, $award_name_link). "\n";
    
    
    $node->body[$node->language][] = array("format" => "full_html", "value" => $background);
    
    $node->field_award_application_process[$node->language][] = array("format" => "full_html", "value" => $application_process);
		$node->field_award_conditions[$node->language][] = array("format" => "full_html", "value" => $conditions);
    
    $node->field_award_uni_trust_record[$node->language][] = array("title" => $trust, "url" => $trust_link);
    $node->field_award_financial_need[$node->language][] = array("value" => $finanicial_need);
    $node->field_award_travel_support[$node->language][] = array("value" => $travel_support);
    $node->field_award_term[$node->language][] = array("value" => $term);    
    
    $node->field_award_value[$node->language][] = array("value" => $award_value);
		$node->field_award_other[$node->language][] = array("format" => "plain_text", "value" => $other);
    $node->field_award_submissions[$node->language][] = array("url" => $apply_link);
    $node->field_award_enquire_now[$node->language][] = array("email" => $enquiry_email);
    
    $node->field_award_application_date[$node->language][] = array("value" => $open_day, "value2" => $close_day);
    $node->field_syndication_site[$node->language][] = array("tid" => $site_term_id);
    
    
    // Award type
    foreach($award_type_array as $award_type)
    {
    	$node->field_award_type[$node->language][] = array("value" => $award_type);
		}
		
    // Study level
    foreach($study_level_array as $study_level)
    {
    	$node->field_award_study_level[$node->language][] = array("value" => $study_level);
		}
		
		// Citizenship
    foreach($citizenship_array as $citizenship)
    {
    	$node->field_award_citizenship[$node->language][] = array("value" => $citizenship);
		}
		
		// Area of study
		foreach($study_area_array as $study_area_id)
    {
    	$node->field_award_study_area[$node->language][] = array("tid" => $study_area_id);
		}

		// Student type
		foreach($student_type_array as $student_type)
    {
    	$node->field_award_students[$node->language][] = array("value" => $student_type);
		}
		
		// Frequency
		foreach($frequency_array as $frequency)
    {
    	$node->field_award_frequency[$node->language][] = array("value" => $frequency);
		}
		
		node_save($node);
		
		//test
    //echo "\nrow: $row, frequency: $frequency, node_id: $node->nid\n";
	}
	
	
	// Report
	$runtime_end = getrusage();
	$utime = _output_runtime($runtime_end, $runtime_start, "utime");
	$stime = _output_runtime($runtime_end, $runtime_start, "stime");
	
	$report = "
		column: $highest_column, row: $highest_row\n
		This process used ". $utime. " ms for its computations\n
		It spent ". $stime. " ms in system calls\n
	";

	echo dt($report);
}


function drush_delete_nodes($node_type = '')
{
	if(empty($node_type)) 
	{
    drush_set_error('NO_ARGUMENT', dt('I require node_type'));
    return 1;
  }

	
	// Select the nodes that we want to delete.
	$result = db_select('node', 'n')
		->fields('n', array('nid'))
		->condition('type', $node_type, '=')
		->execute();

	$deleted_count = 0;
	foreach($result as $record) 
	{
		node_delete($record->nid);
		$deleted_count++;
	}

	// Simple debug message so we can see what had been deleted.
	drush_print( 
		dt("node_type: $node_type | $deleted_count nodes have been deleted") 
	);
}

// Delete all award nodes, except the ones defined
function drush_delete_awards()
{
	$exception_titles = array(
		"A.F. Davies Prize",
		"AIRC Centennial Prize",
		"Alma Hansen Scholarship",
		"Amy Gaye Cowper Tennent Memorial Scholarship",
		"Anna Lodewyckx Scandinavian Icelandic Scholarships",
		
		"Caroline Kay Scholarship",
		"Elizabeth and Nicholas Slezak Scholarship",
		"Emma Grollo Memorial Scholarship",
		"Ernest Scott Prize",
		"Felix Meyer Scholarship",		
				
		"French History Research Higher Degree Scholarship",
		"Harold Wright Scholarship and Sarah & William Holmes Scholarship",
		"HB Higgins Poetry Scholarship",
		"Jessie Webb Scholarship",
		"John Masefield Prize",
		
		"Lizette Bentwitch Scholarship",
		"Margaret Cohan Research Scholarships",
		"Miltiades and Alkistis Chryssavgis Scholarship",
		"Norman Macgeorge Scholarship",
		"The George Andrew Bailey Memorial Medieval Travelling Scholarship",
		
		"The Gwenda Ford English Literature Scholarship",
		"The Igor Meshakov-Korjakin Russian Scholarship",
		"The S.Ernest Sprott Fellowship",
		"The University of Melbourne French Trust Fund",
		"Ursula Hoff Art History Scholarship",
		
		"Wesley Michel Wright Prizes in Poetry",
		"WT Mollison Scholarship",
		"Wyselaskie Scholarship in Modern British History",
	);


	$result = db_select('node', 'n')
		->fields('n')
		->condition('type', 'award', '=')
		->execute();

	$keep_counter = 0;
	$delete_counter = 0;

	foreach($result as $node)
	{
		if(in_array($node->title, $exception_titles))
		{
			echo "\nkeep $node->nid | $node->title\n";
			++$keep_counter;
		}
		else
		{
			node_delete($node->nid);
			echo "\ndelete $node->nid | $node->title\n";
			++$delete_counter;
		}
	}
	
	echo "\ntotal keep: $keep_counter | total delete: $delete_counter\n";
}

function _drush_scrape_deindex($text) {
  return strtr($text, array('/index.html' => ''));
}

function drush_scrape_get_links($url, $page, $content) {
  $links = array();
  foreach ($page->find($content . ' a') as $link) {
    // Absolute local link.
    if ($link->href[0] == '/') {
      // Prepend the hostname part of the URL only.
      $links[$link->href] = _drush_scrape_deindex($link->href);
    }
  }
  return $links;
}

/**
 * Command callback.
 */
function drush_scrape_slurp($url = '') {
  if (empty($url)) {
    drush_set_error('NO_URL', dt('I require a URL as argument.'));
    return 1;
  }

  $uri = parse_url($url);
  $element   = trim(drush_get_option('element'));
  if (!empty($element)) {
    $element .= ' ';
  }
  $path_only = drush_get_option('path-only');

  // drush_print(dt('Element: %element', array('%element' => $element)));
  // drush_print(dt('URI: @array', array('@array' => print_r($uri, TRUE))));

  require_once('simple_html_dom.inc');

  // Grab etc.
  $page = file_get_html($url);
  if (empty($page)) {
    drush_set_error('NO_CONTENT', dt('The url @url returned no content.', array('@url' => $url)));
    return 1;
  }

  foreach ($page->find($element . 'a') as $link) {
    // Check if these links point at the same server. Ignore if not.
    $temp = parse_url($link->href);
    if ($temp['host'] != $uri['host']) {
      continue;
    }

    // Return only the path component.
    $links[$temp['path']] = array(
      'old' => $temp['path'],
      'new' => _drush_scrape_deindex($temp['path']),
      'title' => trim($link->plaintext),
    );
  }

  foreach ($links as $link) {
    drush_print(dt('@old,@new,"!title"',
      array(
        '@old' => $link['old'],
        '@new' => $link['new'],
        '!title' => $link['title']
      )
    ));
  }
}


function _award_get_multiple_cell_values(&$worksheet_obj, $initial_col_string, $col_range, $row)
{
	$return_array = array();
	
	$col = PHPExcel_Cell::columnIndexFromString($initial_col_string) - 1;
	for($i=0; $i < $col_range; $i++) 
	{
		$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
		$cell_value = $cell->getValue();
		
		if(!empty($cell_value))
		{
			$curr_col_string = PHPExcel_Cell::stringFromColumnIndex($col);
			$return_array[] = _award_interpret_spreadsheet_column($curr_col_string);
		}
		else
		{
			// Skip
		}
		
		++$col;
	}
	
	return $return_array;
}

function _award_get_cell_value_bicondition(&$worksheet_obj, $col_string, $row)
{
	$return = "";

	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$value = $cell->getValue();

	if(empty($value))
	{
		$return = "no";
	}
	else
	{
		$return = "yes";
	}
	
	return $return;
}


function _award_interpret_spreadsheet_column($col)
{
	$return = null;

	if(0)
	{
	
	}
	elseif($col == "F")
	{
		$return = "scholarship";
	}
	elseif($col == "G")
	{
		$return = "prize";
	}
	elseif($col == "H")
	{
		$return = "grant";
	}
	elseif($col == "P")
	{
		$return = "undergraduate";
	}
	elseif($col == "Q")
	{
		$return = "honours";
	}
	elseif($col == "R")
	{
		$return = "graduate";
	}
	elseif($col == "S")
	{
		$return = "postgraduate";
	}
	elseif($col == "T")
	{
		$return = "RHD";
	}
	elseif($col == "U")
	{
		$return = "fellowship";
	}
	elseif($col == "V")
	{
		$return = array("au", "nz");
	}
	elseif($col == "W")
	{
		$return = array("au", "nz", "int");
	}
	elseif($col == "X")
	{
		$return = array("int");
	}
	elseif($col == "AF")
	{
		$return = "commencing";
	}
	elseif($col == "AG")
	{
		$return = "current";
	}
	elseif($col == "AH")
	{
		$return = "not enrolled";
	}
	elseif($col == "AN")
	{
		$return = "biannual";
	}
	elseif($col == "AO")
	{
		$return = "annual";
	}
	elseif($col == "AP")
	{
		$return = "biennial";
	}
	elseif($col == "AQ")
	{
		$return = "see conditions";
	}
	
	
	return $return;
}

function _award_get_cell_value(&$worksheet_obj, $col_string, $row)
{
	$return = null;

	// See legacy code of phpexcel e.g. B == 2
	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$value = $cell->getValue();
	if($cell->hasHyperlink())
	{
		$link = $cell->getHyperlink()->getUrl();
	}
	else
	{
		$link = null;
	}	

	$return = array(
		"value" => $value,
		"link" => $link
	);

	return $return;
}

function _award_get_numeric_cell_value($worksheet_obj, $col_string, $row)
{
	$return = null;

	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$value = $cell->getValue();
	if(is_numeric($value))
	{
		$return = $value;
	}
	else
	{
		$return = 0;
	}
	
	return $return;
}

function _award_get_study_area_table_ids(&$worksheet_obj, $col_string, $row)
{
	$return_array = array();

	$study_area_in_cell = _award_get_study_area_in_cell($worksheet_obj, $col_string, $row);
	$study_area = _award_get_study_area_array();	

	if(count($study_area_in_cell) > 0)
	{
		if($study_area_in_cell[0] === "all")
		{
			foreach($study_area as $key => $value)
			{
				$return_array[] = $key;	
			}
		}
		else
		{
			foreach($study_area_in_cell as $study_area_needle)
			{
				if( ($key = array_search($study_area_needle, $study_area)) !== FALSE )
				{
					$return_array[] = $key;
					//echo "\ngood search $study_area_needle\n";
				}
				else
				{
					//echo "\nbad search $study_area_needle\n";
				}
			}
		}
	}
	
	
	//test
	/*
	echo "\n-start-";
	print_r($study_area_in_cell);
	print_r($study_area);
	print_r($return_array);
	echo "-end-\n";
	*/
	
	return $return_array;
}

// Values in a cell looks like: 
// Asia Institute|Asian studies|Culture and Communication|Art history
function _award_get_study_area_in_cell(&$worksheet_obj, $col_string, $row)
{
	$return = null;

	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$value = $cell->getValue();

	$tmp_array = explode("|", $value);
	
	foreach($tmp_array as $item)
	{
		$return[] = strtolower($item);
	}
	
	return $return;
}

function _award_get_study_area_array()
{
	$return_array = array();

	$query = db_select("taxonomy_term_data", "term");
	$query->join("taxonomy_vocabulary", "voca", "term.vid = voca.vid");
	$result = $query
  	->fields("term", array("tid", "name"))
  	->condition("voca.machine_name", "study_area")
  	->orderBy("tid", 'ASC')
  	->execute();

	foreach($result as $data)
	{
		$return_array[$data->tid] = strtolower($data->name); 
	}
	
	return $return_array;
}

function _award_get_date_in_cell(&$worksheet_obj, $col_string, $row)
{
	$return = null;

	$output_array = _award_get_cell_value($worksheet_obj, $col_string, $row);
	$date_in_cell = $output_array["value"];
	
	if(!empty($date_in_cell))
	{
		$date = PHPExcel_Shared_Date::ExcelToPHPObject($date_in_cell);
		$return = $date->format('Y-m-d');
	}
	else
	{
	
	}

	return $return;
}

function _award_get_default_site($site_name = "")
{
	$query = db_select("taxonomy_term_data", "term");
	$query->join("taxonomy_vocabulary", "voca", "term.vid = voca.vid");
	$result = $query
  	->fields("term", array("tid"))
  	->condition("voca.machine_name", "site")
  	->condition("term.name", $site_name)
  	->orderBy("tid", 'ASC')
  	->execute()
  	->fetchAssoc();

	return $result["tid"];
}

function _is_no_award_type(&$worksheet_obj, $col_string, $col_range, $row)
{
	$condi = false;
	
	$value_array = _award_get_multiple_cell_values($worksheet_obj, $col_string, $col_range, $row);
	
	if(count($value_array) == 0)
	{
		$condi = true;
	}
	else
	{
		$condi = false;
	}

	return $condi;
}


function _output_runtime($ru, $rus, $index)
{
	return ($ru["ru_$index.tv_sec"]*1000 + intval($ru["ru_$index.tv_usec"]/1000)) - 
		($rus["ru_$index.tv_sec"]*1000 + intval($rus["ru_$index.tv_usec"]/1000));
}

function _flatten_array($array) 
{ 
  if(!is_array($array)) 
  { 
    return FALSE; 
  } 
  
  $result = array(); 
  foreach($array as $key => $value) 
  { 
    if(is_array($value)) 
    { 
    	// If the input arrays have the same string keys, then the later value for that key will overwrite the previous one. If, however, the arrays contain numeric keys, the later value will not overwrite the original value, but will be appended.
      $result = array_merge($result, _flatten_array($value)); 
    } 
    else 
    { 
      $result[$key] = $value; 
    } 
  } 
  return $result; 
}

function _destill_field_content($string)
{
	$return_string = "";

	// Too short then remove it
	if($string === "•")
	{
		$return_string = "";
	}
	else
	{
		$return_string = $string;
	}

	//echo "\ntest: ". $string. "|". strlen($string). "\n";

	return $return_string;
}

function _get_valid_url($url)
{
	$return_url = "";

	if(valid_url($url, TRUE))
	{
		$return_url = $url;
	}
	else
	{
		$return_url = "";
	}

	return $return_url;
}
