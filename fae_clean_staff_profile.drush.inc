<?php


/**
 * Implements hook_drush_command().
 */
function fae_clean_staff_profile_drush_command() {
  $items['fae_clean_staff_profile'] = array(
    'callback' => 'fae_clean_staff_profile',
    'description' => dt('fae_clean_staff_profile'),
    'arguments' => array(
      'host' => dt('The host e.g. http://cc.dev.arts.unimelb.edu.au'),
    ),
    'examples' => array(
      'simple' => dt('drush fae_clean_staff_profile http://cc.dev.arts.unimelb.edu.au'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  return $items;
}


function fae_clean_staff_profile($host) {
	// Note:
	// $host == http://cc.dev.arts.unimelb.edu.au

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
	//$i = 0;
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

			/*
			// Test	  
		  if($i == 2) {
		  	break;
		  }
		  else {
		  	++$i;
		  }
			*/

			// Avoid crash joseki 
    	sleep(4);
		}

	}
	
	curl_close($curl);
}
