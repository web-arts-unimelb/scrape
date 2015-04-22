<?php


/**
 * Implements hook_drush_command().
 */
function fae_clean_staff_profile_drush_command() {
  $items['fae_clean_staff_profile'] = array(
    'callback' => 'fae_clean_staff_profile',
    'description' => dt('fae_clean_staff_profile'),
		/*
    'arguments' => array(
      'host' => dt('The host e.g. http://cc.dev.arts.unimelb.edu.au'),
    ),
		*/
    'examples' => array(
      'simple' => dt('drush fae_clean_staff_profile'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}


function fae_clean_staff_profile() {
	// Turn on test on small data
	$_is_small_data_test = FALSE; 

	_fae_clean_staff_msg("Start");
	
	global $base_url;
	$host = $base_url;
	$lang = LANGUAGE_NONE;
	$users = entity_load('user');

	$curl = curl_init();
	
	$agent = 'Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.0.3705; .NET CLR 1.1.4322)';
	curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
	//curl_setopt($curl, CURLOPT_NOBODY, true);
	curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($curl, CURLOPT_USERAGENT, $agent);
	
	// Prepare log file
	// http://stackoverflow.com/questions/10000494/remove-protocol-and-result-with-full-base-url-from-html-input-string
	preg_match('%[^/]+\.[^/:]{2,3}%m', $host, $matches);
	$domain = $matches[0];

	$log_file = $domain. '_'. 'fae_clean_staff_profile.log';
	file_put_contents($log_file, '');	

	//Test
	if($_is_small_data_test) {
		$i = 0;
	}	

	_fae_clean_staff_msg("Simulate user visits");	
	foreach($users as $user_id => $user) {
		if(
			isset($user->field_fae_resource[$lang][0]['url']) &&
			!empty($user->field_fae_resource[$lang][0]['url'])
		) {
			$url = $host. "/user/". $user_id;
			
			// Output
			$msg = date('Y-m-d H:i:s'). ' hitting: '. $url. PHP_EOL; 
			file_put_contents($log_file, $msg, FILE_APPEND | LOCK_EX);				
	
			curl_setopt($curl, CURLOPT_URL, $url);
		  $result = curl_exec($curl);

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
			// I hold a timer and test it.
			$time = 120;
    	sleep($time);
		}

	}
	
	curl_close($curl);

	_fae_clean_staff_msg("Making clean data");	
	_fae_clean_staff_copy_data();

	_fae_clean_staff_msg("End");
}

function _fae_clean_staff_msg($msg) {
	echo "\n-". $msg. "-\n";
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
