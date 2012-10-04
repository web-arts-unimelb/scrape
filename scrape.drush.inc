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
    ),
    'examples' => array(
      'simple' =>  dt('drush scape filename.txt'),
      'with url' => dt('drush scape --url=http://ltrc.unimelb.edu.au filename.txt'),
      'complex' => dt('drush scape --source-url=http://ltrc.unimelb.edu.au --source-title=title --source-content=div[id=content] --target-node=page --target-files=field_attachment filename.txt'),
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

    if (!empty($source_url)) {
      $url = $source_url . $uri;
    }

    // Grab etc.
    $page = file_get_html($url);
    if (empty($page))
      continue;

    $uri = parse_url($url);

    $page_title = trim($page->find($title, 0)->plaintext);
    $page_body = trim($page->find($content, 0));
    $page_files = drush_scrape_get_files($url, $page, $content);

    // Loop through the page body and add the links to drupal-hosted files.
    $replacements = array();
    $files = array();
    foreach ($page_files as $link => $file) {
      $furl = file_create_url($file['uri']);
      $furi = drupal_parse_url($furl);
      $replacements[$link] = $furi['path'];
      $files[] = $file;
    }
    $page_body = strtr($page_body, $replacements);

    $node = new stdClass();
    $node->type = $target_node;
    $node->nid = NULL;
    $node->uid = 1;
    $node->title = check_plain($page_title);
    $node->language = LANGUAGE_NONE;
    $node->body[$node->language] = array(0 => array('format'=> 'full_html', 'value' => $page_body));
    $node->path = array('alias' => substr($uri['path'], 1));
    $node->{$target_files}[$node->language] = $files;

    node_save($node);

    drush_print(dt('Saved @title as @type @nid (@alias)', array(
      '@title' => $page_title,
      '@type' => $target_node,
      '@nid' => $node->nid,
      '@alias' => $node->path['alias'])
    ));

    $page->clear();
  }
  fclose($fp);
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
