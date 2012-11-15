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
      'simple' =>  dt('drush scape filename.txt'),
      'with url' => dt('drush scape --url=http://ltrc.unimelb.edu.au filename.txt'),
      'complex' => dt('drush scape --source-url=http://ltrc.unimelb.edu.au --source-title=title --source-content=div[id=content] --target-node=page --target-files=field_attachment --target-images=field_image filename.txt'),
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

	// Import award in filemaker to drupal node
	$items['import_prize'] = array(
		'callback' => 'drush_import_prize',
    'description' => dt('Import a well-formatted equipment CSV to award nodes.'),
    'arguments' => array(
      'input' => dt('File containing a list of prizes.'),
    ),
    'examples' => array(
      'simple' => dt('drush import_prize /full_path/filename.csv'),
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


function drush_import_prize($input = '') 
{
	if(empty($input)) 
	{
    drush_set_error('NO_FILE', dt('I require a CSV file with a list of columns.'));
    return 1;
  }
  
  if(!file_exists($input)) 
  {
    drush_set_error('NO_FILE', dt('List file "@file" does not exist.', array('@file'=> $input)));
    return 1;
  }

  $fp = fopen($input, 'r');
  if(!$fp) 
  {
    drush_set_error('FILE_PERM', dt('Cannot open file @file.', array('@file'=> $input)));
  }

	// Variables
	$node_type = "award";
	$line_size = 2048;

	while(!feof($fp))
	{
		$data = fgetcsv($fp, $line_size);
	
		$title = utf8_encode($data[0]);
		$background = utf8_encode($data[1]);
	
		$replacement = array(
			"" => "<br/>",
			"" => "'",
			"" => "'",
			"" => "'",
			"" => "'", 
			"" => "-"
		);
		$background = strtr($background, $replacement);
		
	
		// No data, skip.
    if(!count($data))
      continue;
	
		// If title is empty, skip
		if(empty($title))
			continue;

		$node = new stdClass();
    $node->type = $node_type;
    $node->nid = NULL;
    $node->uid = 1;
    $node->title = $title;
    $node->language = LANGUAGE_NONE;
    $node->body[$node->language] = array(0 => array('format'=> 'full_html', 'value' => $background));

    node_save($node);

		/*
    drush_print(
    	dt('title: @title | node_id: @nid', 
    		array(
      		'@title' => $title,
      		'@nid' => $node->nid
    		)
    	)
    );
    */
    drush_print(
    	dt('node_id: @nid | text: @text', 
    		array(
      		'@nid' => $node->nid,
      		'@text' => $background
    		)
    	)
    );
    
	}
	// End while loop
	
	fclose($fp);
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
    $links[$link->href] = _drush_scrape_deindex($temp['path']);
  }

  foreach ($links as $link) {
    drush_print(dt('@link', array('@link' => $link)));
  }
}
