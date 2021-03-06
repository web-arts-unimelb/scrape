<?php


/**
 * Implements hook_drush_command().
 */
function gen_redirect_drush_command() {
  $items['gen_redirect'] = array(
    'callback' => 'drush_gen_redirect',
    'description' => dt('Generate redirects, which are put into .htaccess'),
    'aliases' => array('gr', 'redir', 'rewrite'),
    'arguments' => array(
      'host' => dt('The host e.g. arts.unimelb.edu.au'),
      'input_file' => dt('A csv file in formate as old_url, new_url'),
    ),
    'examples' => array(
      'simple' => dt('drush gen_redirect arts.unimelb.edu.au /full_path/filename.csv > out.txt'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  return $items;
}


function drush_gen_redirect($host = "", $input_file = "") {
  if (empty($host)) {
    drush_set_error('NO_HOST', dt('I need the host e.g. arts.unimelb.edu.au'));
    return 1;
  }

  if (!file_exists($input_file)) {
    drush_set_error('NO_FILE', dt('"@file" does not exist.', array('@file' => $input_file)));
    return 1;
  }

  $fp = fopen($input_file, 'r');
  $escape = array(
    '.' => '\.',
    '-' => '\-',
  );

	// If $old_url contains full url, remove output directory, see _handle_full_url for why.
  $output_dir_path = 'output';
  _remove_dir_files($output_dir_path);

  while (!feof($fp)) {
    list($old_url, $new_url) = fgetcsv($fp, 1024);

    // Trim
    $old_url = trim($old_url);
    $new_url = trim($new_url);

    $protocol = "http://"; // Assume
    $site_url = $protocol . $host;

    if (_is_skipped($old_url, $new_url)) {
      continue;
    }

		if (_is_full_url($old_url)) {
			_handle_full_url($old_url, $new_url, $host);
			continue; // Skip the rest
		}

    // Destill
    $old_url = _destill_old_uri($old_url);

    printf("RewriteCond %%{HTTP_HOST} ^%s$ [NC]\n", strtr($host, $escape));

    if (_gen_redirect_valid_url($new_url, TRUE)) {
      printf("RewriteRule ^%s?$ %s [R=301,L]\n", strtr($old_url, $escape), $new_url);
    }
    elseif (_is_script($old_url)) {
      printf("RewriteRule ^%s$ $site_url%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
    }
    elseif (_is_end_with_forward_slash($old_url)) {
      printf("RewriteRule ^%s?$ $site_url%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
    }
    else {
      printf("RewriteRule ^%s/?$ $site_url%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
    }
  }
  fclose($fp);
}

function _is_skipped($old_url = "", $new_url = "") {
  $condi = FALSE;

  $case_pattern = "/^(?=.*[A-Z]).*$/";
  $php_pattern = "/(\.php)$/";

  // Skip empty fields.
  if (empty($old_url) || empty($new_url)) {
    $condi = TRUE;
  }
  if (preg_match('/^\/$/', $old_url)) {
    $condi = TRUE;
  }
  elseif (!_gen_redirect_valid_url($old_url)) {
    $condi = TRUE;
  }
  elseif (!_gen_redirect_valid_url($new_url)) {
    $condi = TRUE;
  }
  elseif (preg_match($case_pattern, $old_url)) {
    // old_url contains upper case
    $condi = TRUE;
  }
  elseif (preg_match($case_pattern, $new_url)) {
    // new_url contains upper case
    $condi = TRUE;
  }
  elseif (preg_match($php_pattern, $old_url)) {
    // NOTE: If php script, skip
    $condi = TRUE;
  }
  elseif (_is_only_slath_difference($old_url, $new_url)) {
    $condi = TRUE;
  }

  return $condi;
}

function _is_script($url = "") {
  $pattern = "/(\.[^.]+)$/";
  if (preg_match($pattern, $url)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

function _is_end_with_forward_slash($url = "") {
  $pattern = "/(\/){1}$/";
  if (preg_match($pattern, $url)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

function _is_only_slath_difference($old_url = "", $new_url = "") {
  $old_url = preg_replace('{/$}', '', $old_url);
  $old_url = preg_replace('{^/}', '', $old_url);

  $new_url = preg_replace('{/$}', '', $new_url);
  $new_url = preg_replace('{^/}', '', $new_url);

  if ($old_url === $new_url) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

function _destill_old_uri($uri) {
  // Remove leading "/" if there is one
  $uri = ltrim($uri, '/');

  return $uri;
}

function _gen_redirect_valid_url($url, $absolute = FALSE) {
  if ($absolute) {
    return (bool) preg_match("
      /^                                                      # Start at the beginning of the text
      (?:ftp|https?|feed):\/\/                                # Look for ftp, http, https or feed schemes
      (?:                                                     # Userinfo (optional) which is typically
        (?:(?:[\w\.\-\+!$&'\(\)*\+,;=]|%[0-9a-f]{2})+:)*      # a username or a username and password
        (?:[\w\.\-\+%!$&'\(\)*\+,;=]|%[0-9a-f]{2})+@          # combination
      )?
      (?:
        (?:[a-z0-9\-\.]|%[0-9a-f]{2})+                        # A domain name or a IPv4 address
        |(?:\[(?:[0-9a-f]{0,4}:)*(?:[0-9a-f]{0,4})\])         # or a well formed IPv6 address
      )
      (?::[0-9]+)?                                            # Server port number (optional)
      (?:[\/|\?]
        (?:[\w#!:\.\?\+=&@$'~*,;\/\(\)\[\]\-]|%[0-9a-f]{2})   # The path and query (optional)
      *)?
      $/xi", $url);
  }
  else {
    return (bool) preg_match("/^(?:[\w#!:\.\?\+=&@$'~*,;\/\(\)\[\]\-]|%[0-9a-f]{2})+$/i", $url);
  }
}

function _is_full_url($url) {
	$http_pattern = "|^https?://|";

	if(preg_match($http_pattern, $url)) {
 		return true; 
  }
	else {
		return false;
	}
}

function _handle_full_url($old_url, $new_url, $migrated_hostname) {
	/*
		- So assume we have $old_url: http://classics-archaeology.unimelb.edu.au/research/research-facilities/library.html?arg=value#anchor
	 		$new_url: /classics-archaeology/library 

		- Or $old_url: http://classics-archaeology.unimelb.edu.au
			$new_url: /classics-archaeology 
	*/

	$part = parse_url($old_url);
	$scheme = $part['scheme']; // e.g. http
	$hostname = $part['host']; // e.g. classics-archaeology.unimelb.edu.au
	$path = $part['path']; // e.g. /research/research-facilities/library.html
	$query = $part['query']; // e.g. arg=value
	$fragment = $part['fragment']; // e.g. anchor
	
	if(!empty($query)) {
		$needed_query = '?'. $query;
	}
	else {
		$needed_query = '';
	}

	if(!empty($fragment)) {
    $needed_fragment = '#'. $fragment;
  }
  else {
    $needed_fragment = '';
  }

	// e.g. /research/research-facilities/library.html?arg=value#anchor
	$needed_old_uri = $path. $needed_query. $needed_fragment;

	if(empty($needed_old_uri)) {
		$tmp_needed_old_uri = '/';
	}
	else {
		$tmp_needed_old_uri = $needed_old_uri;
	}

	// Destill
  $tmp_needed_old_uri = _destill_old_uri($tmp_needed_old_uri);

	$escape = array(
    '.' => '\.',
    '-' => '\-',
  );

	$file_name = 'output/'. $hostname. '.txt';
	$fp = fopen($file_name, 'a');
	$needed_migrated_hostname = 'http://'. $migrated_hostname; // Assume http only

	// Ignore for now
	//fprintf($fp, "RewriteCond %%{HTTP_HOST} ^%s$ [NC]\n", strtr($hostname, $escape));

  if (_gen_redirect_valid_url($new_url, TRUE)) {
		// $new_url is full url 
		fprintf($fp, "RewriteRule ^%s?$ %s [R=301,L]\n", strtr($tmp_needed_old_uri, $escape), $new_url);
  }
  elseif (_is_script($tmp_needed_old_uri)) {
		// Is a script
  	fprintf($fp, "RewriteRule ^%s$ $needed_migrated_hostname%s [R=301,L]\n", strtr($tmp_needed_old_uri, $escape), $new_url);
  }
  elseif (_is_end_with_forward_slash($tmp_needed_old_uri)) {
    fprintf($fp, "RewriteRule ^%s?$ $needed_migrated_hostname%s [R=301,L]\n", strtr($tmp_needed_old_uri, $escape), $new_url);
  }
  else {
    fprintf($fp, "RewriteRule ^%s/?$ $needed_migrated_hostname%s [R=301,L]\n", strtr($tmp_needed_old_uri, $escape), $new_url);
  }

	fclose($fp);
}

function _remove_dir_files($path) {
	$files = glob($path. '/*'); // get all file names
	foreach($files as $file){ // iterate files
  	if(is_file($file)) {
    	unlink($file); // delete file
		}
	}
}
