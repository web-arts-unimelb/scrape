<?php
// This script is used to wipe out all 3 find an expert cache table
// and regenerate them. NOTE that the coping tables below is not working,
// so need to do it manually.
$lib_path = dirname(__FILE__). "/lib/lib.php";
include_once($lib_path); 

$devsite_credential_path = dirname(__FILE__). "/config/devsite_credential.config.php";
include_once($devsite_credential_path);

$this_script_config_path = dirname(__FILE__). "/config/fae_clean_staff_profile.config.php";
include_once($this_script_config_path);

// NOTE: need to turn off anonyous user visit cache in drupal 7
// Avoid x-drupal-cache

/**
 * Implements hook_drush_command().
 */
function fae_clean_staff_profile_drush_command() {
  $items['fae_clean_staff_profile'] = array(
    'callback' => 'fae_clean_staff_profile',
    'description' => dt('fae_clean_staff_profile'),
    'examples' => array(
      'simple' => dt('drush fae_clean_staff_profile'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}


function fae_clean_staff_profile() {
	global $_is_small_data_test; 
	global $base_url; // e.g. http://dev.site.com

	my_msg("Remeber to turn off the anonymous user cache in Drupal before running this script, then laster turn the cache back on");
	my_msg("Start");
	
	$host = $base_url;
	$lang = LANGUAGE_NONE;
	$users = entity_load('user');

	
	$agent = 'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
  //curl_setopt($ch, CURLOPT_VERBOSE, true);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  curl_setopt($ch, CURLOPT_USERAGENT, $agent);

	// Prepare log file
	// http://stackoverflow.com/questions/10000494/remove-protocol-and-result-with-full-base-url-from-html-input-string
	preg_match('%[^/]+\.[^/:]{2,3}%m', $host, $matches);
	$domain = $matches[0];

	// Create empty file
	$log_file = $domain. '_'. 'fae_clean_staff_profile.log';
	file_put_contents($log_file, '');	

	//Test
	if($_is_small_data_test) {
		$i = 0;
	}	
	else {
		$i = 0; // Duplicated
	}

	my_msg('Tuncate tables: fae_profile_custom_cache, fae_profile_custom_cache_sync, fae_profile_custom_cache_clean');
	_fae_clean_staff_truncate();

	my_msg("Simulate user visits");	
	foreach($users as $user_id => $user) {
		if(
			isset($user->field_fae_resource[$lang][0]['url']) &&
			!empty($user->field_fae_resource[$lang][0]['url'])
		) {
			$url = _fae_clean_staff_build_host($domain). "/user/". $user_id;
		
			// Output
			$msg = date('Y-m-d H:i:s'). ' hitting: '. $url. PHP_EOL; 
			file_put_contents($log_file, $msg, FILE_APPEND | LOCK_EX);				
	
			curl_setopt($ch, CURLOPT_URL, $url);

			// NOTE: need to turn off anonyous user visit cache in drupal 7
			// Avoid x-drupal-cache
		  $result = curl_exec($ch);
			//print_r($result);

			// Test
			if($_is_small_data_test) {
		  	if($i == 2) {
		  		break;
		  	}
		  	else {
		  		++$i;
		  	}
			}

			// Avoid crash joseki
			// http://php.net/manual/en/function.sleep.php
			// I hold a timer and test it. It took around 20s to fully load.
			$time = 45;
    	sleep($time);
		}

	}
	curl_close($ch);

	// NOTE: It is not working why?
	my_msg("Copy fae_profile_custom_cache to fae_profile_custom_cache_sync");
	_fae_clean_staff_copy_data('fae_profile_custom_cache', 'fae_profile_custom_cache_sync');

	// Sleep
	sleep(40);

	// NOTE: It is not working why?
	my_msg("Copy fae_profile_custom_cache to fae_profile_custom_cache_clean");
	_fae_clean_staff_copy_data('fae_profile_custom_cache', 'fae_profile_custom_cache_clean');

	my_msg("End");
}


function _fae_clean_staff_copy_data($table_name_a = "fae_profile_custom_cache", $table_name_b = "fae_profile_custom_cache_clean") {
	// Check
	if(!db_table_exists($table_name_b)) {
    $sql = "
      CREATE TABLE $table_name_b(
        id INT(11) NOT NULL AUTO_INCREMENT,
        cache_id VARCHAR(256) NOT NULL,
        data LONGTEXT NOT NULL,
        PRIMARY KEY ( id )
      )
    ";
    db_query($sql);
  }	

	// Truncate
	$sql = "TRUNCATE TABLE $table_name_b";
	db_query($sql);

	// Load data
	$sql ="INSERT $table_name_b SELECT * FROM $table_name_a";	
	db_query($sql);		
}

function _fae_clean_staff_truncate() {
  $sql = "
    TRUNCATE TABLE fae_profile_custom_cache;
    TRUNCATE TABLE fae_profile_custom_cache_sync;
    TRUNCATE TABLE fae_profile_custom_cache_clean;";

  db_query($sql);
}

function _fae_clean_staff_build_host($domain = '') {
	global $_is_dev;
	global $devsite_username;
  global $devsite_password;

	$return_host = '';
	$protocol = 'http://'; // Assume

	if($_is_dev) {
		$return_host = $protocol. $devsite_username. ':'. $devsite_password. '@'. $domain; 
	}
	else {
		$return_host = $protocol. $domain;
	}

	return $return_host;
}
