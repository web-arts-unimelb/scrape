<?php

// This is a test.
$lib_path = dirname(__FILE__). "/lib/lib.php";
include_once($lib_path);

$devsite_credential_path = dirname(__FILE__). "/config/devsite_credential.config.php";
include_once($devsite_credential_path);

/**
 * Implements hook_drush_command().
 */
function fae_clean_staff_test_drush_command() {
  $items['fae_clean_staff_test'] = array(
    'callback' => 'fae_clean_staff_test',
    'description' => dt('fae_clean_staff_test'),
    'examples' => array(
      'simple' => dt('drush fae_clean_staff_test'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}

// NOTE:
// When examine curl and set verbose to true, I discover X-Drupal-Cache: HIT or MISS.
// There is a setting called "Cache pages for anonymous users" in Drupal 7.
// I need to uncheck, so page won't cache any time.
function fae_clean_staff_test() {
	global $devsite_username;
	global $devsite_password;

	my_msg('-Start-');

	//_fae_clean_staff_test_clean();	
	$url = "http://$devsite_username:$devsite_password@cc.dev.arts.unimelb.edu.au/user/32";
	//$url = "http://$devsite_username:$devsite_password@cc.dev.arts.unimelb.edu.au/about/staff/professor-jaynie-anderson";
	$ch = curl_init();

	// http://stackoverflow.com/questions/2440729/php-curl-how-can-i-emulate-a-get-request-exactly-like-a-web-browser
	// http://docs.php.net/function.curl-setopt
	// my_cookie.txt chmod 777

	$agent = 'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)';

	$ch = curl_init();
	curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
	curl_setopt($ch, CURLOPT_VERBOSE, true);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_USERAGENT, $agent);
	curl_setopt($ch, CURLOPT_URL,$url);
	$result=curl_exec($ch);

	print_r($result);
	my_msg('-End-');
}

function _fae_clean_staff_test_clean() {
	$sql = "
		TRUNCATE TABLE fae_profile_custom_cache;
		TRUNCATE TABLE fae_profile_custom_cache_sync;
		TRUNCATE TABLE fae_profile_custom_cache_clean;";
	
	db_query($sql);
}
