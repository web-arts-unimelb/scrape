<?php
/**
 * @file
 * Drush plugin to import spreadsheet data into drupal site
 */

/**
 * Implements hook_drush_command().
 */
function sheet_drush_command() 
{
	// Import award in spreadsheet to drupal node
	$items['import_staff'] = array(
		'callback' => 'drush_import_staff',
    'description' => dt('Import a staff spreadsheet to staff nodes.'),
    'arguments' => array(
      'input' => dt('File containing a list of staff'),
    ),
    'examples' => array(
      'simple' => dt('drush import_staff /full_path/filename.xlsx'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  
  
	return $items;
}


function drush_import_staff($input = "")
{
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
	
	// Adjust for testing
	$row_to_skip = 1;
	//$highest_row = 32;
	
	for($row = 1; $row <= $highest_row; ++$row) 
	{
		if($row <= $row_to_skip)
			continue;

		// Assume it has given name & family name
		$col_string = "D";
		if(_staff_is_row_to_skip($worksheet_obj, $col_string, $row))
			continue;

		// Unit
		$col_string = "A";
		$unit = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Sub unit
		$col_string = "B";
		$sub_unit = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Title
		$col_string = "C";
		$title = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Given name
		$col_string = "D";
		$given_name = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Family name
		$col_string = "E";
		$family_name = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Email
		$col_string = "F";
		$email = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Phone
		$col_string = "I";
		$phone = _staff_get_cell_value($worksheet_obj, $col_string, $row);
    _staff_tweak_phone($phone);
		
		// Room
		$col_string = "J";
		$room = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		
		// Building name
		$col_string = "K";
		$building_name = _staff_get_cell_value($worksheet_obj, $col_string, $row);
		$map_url = _staff_get_building_map_url($building_name);
		
		//echo "\n$unit | $sub_unit | $title | $given_name | $family_name | $email | $phone | $room | $building_name | $map_url\n";
		
		
		// Save node
		$node = new stdClass();
    $node->type = "staff";
    $node->nid = NULL;
    $node->uid = 1;
		
		$full_name = $given_name. " ". $family_name;
		$node->language = LANGUAGE_NONE;
    $node->title = $full_name;
		
		$node->field_staff_title[$node->language][] = array("value" => "");
		$node->field_staff_family_name[$node->language][] = array("value" => $family_name);
		$node->field_staff_given_name[$node->language][] = array("value" => $given_name);
		$node->field_staff_mail[$node->language][] = array("email" => $email);
		
		$node->field_staff_position[$node->language][] = array("value" => $title);
		$node->	field_staff_phone[$node->language][] = array("value" => $phone);
		$node->field_staff_room[$node->language][] = array("value" => $room);
		$node->field_staff_building[$node->language][] = array("title" => $building_name, "url" => $map_url);
		
		$unit_table_ids = _staff_get_unit_table_ids($unit, $sub_unit);
		foreach($unit_table_ids as $unit_table_id)
    {
    	$node->field_staff_unit[$node->language][] = array("tid" => $unit_table_id);
		}
		
		node_save($node);
	
	}
}

	
function _staff_get_cell_value(&$worksheet_obj, $col_string, $row)
{
	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$cell_value = $cell->getValue();
	
	return $cell_value;
}

function _staff_is_row_to_skip(&$worksheet_obj, $initial_col_string, $row)
{
	$condi = false;

	$col = PHPExcel_Cell::columnIndexFromString($initial_col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$cell_1 = $worksheet_obj->getCellByColumnAndRow($col+1, $row);

	// Must have given name & family name
	$cell_value = $cell->getValue();
	$cell_value_1 = $cell_1->getValue();
	
	if(empty($cell_value) && empty($cell_value_1))
	{
		$condi = true;
	}
	else
	{
		$condi = false;
	}
	
	return $condi;
}

function _staff_get_building_map_url($building_name)
{
	$url = null;
	$building_name = strtolower( trim($building_name) );
	$map_url = "http://maps.unimelb.edu.au/parkville/building";

	if($building_name === "old arts")
	{
		$url = $map_url. "/149";
	}
	elseif($building_name === "babel")
	{
		$url = $map_url. "/139";
	}
	elseif($building_name === "old quad")
	{
		$url = $map_url. "/150";
	}
	elseif($building_name === "arts west")
	{
		$url = $map_url. "/148";
	}
	elseif($building_name === "john medley, east")
	{
		$url = $map_url. "/191";
	}
	elseif($building_name === "john medley, west")
	{
		$url = $map_url. "/191";
	}
	
	return $url;
}

function _staff_get_unit_table_ids($unit="", $sub_unit="")
{
	$return_array = array();

	$unit_array = _staff_get_unit_array();
	$unit = strtolower( trim($unit) );
	$sub_unit = strtolower( trim($sub_unit) );

	if(!empty($sub_unit))
	{
		if( ($key = array_search($sub_unit, $unit_array)) !== FALSE )
		{
			$return_array[] = $key;
		}		
	}

  // If members of a subunit, leave the parent unit out. Our taxonomy already
  // knows about that relationship and adding it explicitly here messes up the
  // sort order on the staff lists.
	if(!empty($unit) && empty($return_array))
	{
		if( ($key = array_search($unit, $unit_array)) !== FALSE )
		{
			$return_array[] = $key;
		}		
	}

	//test
	echo "\n-start-";
	print_r($return_array);
	echo "-end-\n";
	
	return $return_array;
}


function _staff_get_unit_array()
{
	$return_array = array();

	$query = db_select("taxonomy_term_data", "term");
	$query->join("taxonomy_vocabulary", "voca", "term.vid = voca.vid");
	$result = $query
  	->fields("term", array("tid", "name"))
  	->condition("voca.machine_name", "units")
  	->orderBy("tid", 'ASC')
  	->execute();

	foreach($result as $data)
	{
		$return_array[$data->tid] = strtolower($data->name); 
	}
	
	return $return_array;
}

/**
 * Helper to turn an extension into a full phone number.
 *
 * @param &$phone
 *   A (possibly) messy phone number string.
 */
function _staff_tweak_phone(&$phone) {
  if (strlen(trim($phone)) < 6) {
    switch (substr($phone, 0, 1)) {
      case 4:
        $phone = '834' . $phone;
        break;

      case 5:
        $phone = '903' . $phone;
        break;
      }
  }
}
