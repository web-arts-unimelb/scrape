<?php
  
  
/**
* Implements hook_drush_command().
*/
function gen_redirect_drush_command()
{
	$items['gen_redirect'] = array(
		'callback' => 'drush_gen_redirect',
    'description' => dt('Generate redirects, which are put into .htaccess'),
    'arguments' => array(
      'host' => dt('The host e.g. http://arts.unimelb.edu.au'),
      'input_file' => dt('A csv file in formate as old_url, new_url'),
    ),
    'examples' => array(
      'simple' => dt('drush gen_redirect http://arts.unimelb.edu.au /full_path/filename.csv > out.txt'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  
	return $items;
}


function drush_gen_redirect($host = "", $input_file = "")
{
	if(empty($host)) 
	{
	  drush_set_error('NO_HOST', dt('I need the host e.g. http://arts.unimelb.edu.au'));
	  return 1;
	}
	
	if(!file_exists($input_file)) 
	{
	  drush_set_error('NO_FILE', dt('"@file" does not exist.', array('@file'=> $input_file)));
	  return 1;
	}

	$fp = fopen($input_file, 'r');
	$escape = array(
  	'.' => '\.',
  );

  while(!feof($fp)) 
  {
  	list($old_url, $new_url) = fgetcsv($fp, 1024);

		if(_is_skipped($old_url, $new_url))
		{
			continue;
		}
		
		printf("RewriteCond %%{HTTP_HOST} ^%s$ [NC]\n", strtr($host, $escape));
		
		if(_is_script($old_url))
		{
			printf("RewriteRule ^%s$ $host%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
		}
		elseif(_is_end_with_forward_slash($old_url))
		{
			printf("RewriteRule ^%s?$ $host%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
		}
		else
		{
			printf("RewriteRule ^%s/?$ $host%s [R=301,L]\n", strtr($old_url, $escape), $new_url);
		}
	}
	fclose($fp);
}

function _is_skipped($old_url = "", $new_url = "")
{
	$condi = false;

	// Skip empty fields.
	if(empty($old_url) || empty($new_url)) 
	{
		$condi = true;
	}

	// If old url is absolute url, skip
	if(valid_url($old_url, true))
	{
		$condi = true;
	}

	if(!valid_url($new_url))
	{
		$condi = true;
	}

	// Contain upper case
	$pattern = "/^(?=.*[A-Z]).*$/";
	if(preg_match($pattern, $old_url))
	{
		$condi = true;
	}
	
	// Contain upper case
	$pattern = "/^(?=.*[A-Z]).*$/";
	if(preg_match($pattern, $new_url))
	{
		$condi = true;
	}
	
	// NOTE: If php script, skip
	$pattern = "/(\.php)$/";
	if(preg_match($pattern, $old_url))
	{
		$condi = true;
	}
	
	return $condi;
}

function _is_script($url = "")
{
	$pattern = "/(\.[^.]+)$/";
	if(preg_match($pattern, $url))
	{
		return true;
	}
	else
	{
		return false;
	}
}

function _is_end_with_forward_slash($url = "")
{
	$pattern = "/(\/){1}$/";
	if(preg_match($pattern, $url))
	{
		return true;
	}
	else
	{
		return false;
	}
}
