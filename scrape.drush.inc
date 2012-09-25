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
    'callback' => 'drush_do_scrape',
    'description' => dt('Scrape content from web pages.'),
    'arguments' => array(
      'list' => dt('File containing a list of URIs to scape.'),
    ),
    'options' => array(
      'url' => dt('Host URL to prepend to all URIs in file.'),
      'title' => dt('Specify an element selector for the page title.'),
      'content' => dt('Specify an element selector for the page content.'),
    ),
    'examples' => array(
      'simple' =>  dt('drush scape filename.txt'),
      'with url' => dt('drush scape --url=http://ltrc.unimelb.edu.au filename.txt'),
      'complex' => dt('drush scape --url=http://ltrc.unimelb.edu.au --title=title --content=div[id=content] filename.txt'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

/**
 * Implements hook_hook_drush_command_validate().
 */
function drush_do_scrape_validate() {
  if (empty($list)) {
    drush_set_error('NO_FILE', dt('You must specify a filename containing URIs to scrape.'));
  }
  if (!file_exists($list)) {
    drush_set_error('NO_FILE', dt('@file does not exist.', array('@file'=> $file)));
  }
}

/**
 * Command callback.
 */
function drush_do_scrape($list) {
  require_once('simple_html_dom.inc');

  $url = drush_get_option('url');
  $title = drush_get_option('title');
  if (empty($title)) {
    $title = 'title';
  }
  $content = drush_get_option('content');
  if (empty($content)) {
    $content = 'div[id=content]';
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

    if (!empty($url)) {
      $uri = $url . $uri;
    }

    // Grab etc.
    $page = drush_scrape_fetch($uri);
    if (empty($page))
      continue;

    $page_title = trim($page->find($title, 0)->plaintext);
    $page_body = trim($page->find($content, 0));

    drush_print(dt('Fetched page @title', array('@title' => $page_title)));
    drush_print(dt('Fetched body @content', array('@content' => $page_body)));

    $page->clear();
  }
  fclose($fp);

  return $uri;
}

/**
 * Fetch page content from URI.
 */
function drush_scrape_fetch($url) {
  return file_get_html($url);

  $o = curl_init();

  curl_setopt($o, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($o, CURLOPT_URL, $url);
  curl_setopt($o, CURLOPT_USERAGENT, "PHP/cURL; (Linux; Ubuntu 12.04; en-AU); This is ABC/IT scraping your site content with drush");

  $result = curl_exec($o);
  $response = curl_getinfo($o);

  if(curl_errno($o)) {
    drush_print(dt('Curl error @errno: @error', array('@errno' => curl_errno(), '@error' => curl_error($o))));
    return NULL;
  }
  curl_close($o);

  if ($response['http_code'] != '200') {
    return NULL;
  }

  return trim($result);
}
