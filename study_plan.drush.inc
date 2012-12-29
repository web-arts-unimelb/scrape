<?php
  
  
/**
* Implements hook_drush_command().
*/
function study_plan_drush_command() 
{
	$items['import_study_plan'] = array(
		'callback' => 'drush_import_study_plan',
    'description' => dt('Import a study plan list'),
    'arguments' => array(
      'input' => dt('File containing a study plan list'),
    ),
    'examples' => array(
      'simple' => dt('drush import_study_plan /full_path/filename.xlsx'),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );
  
	return $items;
}


function drush_import_study_plan($input = "")
{
	if(empty($input)) 
	{
	  drush_set_error('NO_FILE', dt('I require a spreadsheet file.'));
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

	// Get node
	$query = new EntityFieldQuery();
  $entities = $query->entityCondition('entity_type', 'node')
		->propertyCondition('type', 'webform')
		->propertyCondition('title', 'Complete a study plan check')
		->propertyCondition('status', 1)
		->range(0,1)
		->execute();

  if(!empty($entities['node'])) 
  {
    $node = node_load(array_shift(array_keys($entities['node'])));
  }

	$sheet_num = $excel_obj->getSheetCount();
  for($i=0; $i<$sheet_num; $i++)
  {
  	$excel_obj->setActiveSheetIndex($i);
		$worksheet_obj = $excel_obj->getActiveSheet();
		
		$col_string = "C";
		$row = 2;
		if(_is_subject_sheet($worksheet_obj, $col_string, $row))
		{
			_handle_subject_sheet($worksheet_obj, $node, $i);
		}
		elseif(_is_major_sheet($worksheet_obj, $col_string, $row))
		{
			
		}
		else
		{
		
		}
		
  }

	// Save
	node_save($node);
	
	
	/*
	$query = new EntityFieldQuery();
  $entities = $query->entityCondition('entity_type', 'node')
		->propertyCondition('type', 'webform')
		->propertyCondition('title', 'Complete a study plan check')
		->propertyCondition('status', 1)
		->range(0,1)
		->execute();

  if (!empty($entities['node'])) {
    $node = node_load(array_shift(array_keys($entities['node'])));
  }
	
	$cid = webform_get_cid($node, "an_arts_foundation_subject", 0);
	$items = $node->webform['components'][$cid]['extra']['items'];
	
	$new_item = "one\ntwo\nthree";
	$node->webform['components'][$cid]['extra']['items'] = $new_item;
	
	node_save($node);
	
	print_r($node);
	*/
}


function _handle_subject_sheet(&$worksheet_obj, &$node, $sheet_index)
{
	$highest_row = $worksheet_obj->getHighestRow(); // e.g. it outputs 228
	$row_to_skip = 1;

	$option_item_string = "";
	for($row = 1; $row <= $highest_row; ++$row) 
	{
		if($row <= $row_to_skip)
			continue;	
	
		if(_is_end_of_sheet($worksheet_obj, $row))
			break;

		$col_string = "A";
		$code = _get_cell_value($worksheet_obj, $col_string, $row);
		
		$col_string = "B";
		$title = _get_cell_value($worksheet_obj, $col_string, $row);
		
		$col_string = "C";
		$point = _get_cell_value($worksheet_obj, $col_string, $row);

		$option_item = $code. "--". $title. "--". $point. "\n";
		$option_item_string .= $option_item; 
	}
	$option_item_string = substr($option_item_string, 0, -1);
	
	
	$field_keys = _get_field_keys($sheet_index);
	foreach($field_keys as $field_key)
	{
		$cid = webform_get_cid($node, $field_key, 0);
		$node->webform['components'][$cid]['extra']['items'] = $option_item_string;	
	}
	
}


function _is_subject_sheet(&$worksheet_obj, $col_string = "C", $row = 2)
{
	$cell_value = _get_cell_value($worksheet_obj, $col_string, $row);
	if(!empty($cell_value))
	{
		return true;
	}
	else
	{
		return false;
	}
}

function _is_major_sheet(&$worksheet_obj, $col_string = "C", $row = 2)
{
	$cell_value = _get_cell_value($worksheet_obj, $col_string, $row);
	if(empty($cell_value))
	{
		return true;
	}
	else
	{
		return false;
	}
}


function _is_end_of_sheet(&$worksheet_obj, $row)
{
	$col_string = "A";
	$cell_1 = _get_cell_value($worksheet_obj, $col_string, $row);
	
	$col_string = "B";
	$cell_2 = _get_cell_value($worksheet_obj, $col_string, $row);
	
	$col_string = "C";
	$cell_3 = _get_cell_value($worksheet_obj, $col_string, $row);
	
	if(empty($cell_1) && empty($cell_2) && empty($cell_3))
	{
		return true;
	}
	else
	{
		return false;
	}
}


function _get_field_keys($sheet_index)
{
	$field_keys = array();
	if($sheet_index == 0)
	{
		$field_keys = array("arts_foundation_subject");
	}
	elseif($sheet_index == 1)
	{
		$field_keys = array("arts_foundation_discipline");
	}
	elseif($sheet_index == 2)
	{
		$field_keys = array("discipline_1", "discipline_2", "discipline_3", "discipline_4");
	}
	elseif($sheet_index == 3)
	{
		// Major
	}
	elseif($sheet_index == 4)
	{
		$field_keys = array("major_1_level_1_subject_1", "major_1_level_1_subject_2");
	}
	elseif($sheet_index == 5)
	{
		$field_keys = array("major_1_level_1_subject_3");
	}
	elseif($sheet_index == 6)
	{
		$field_keys = array("major_1_level_2_subject_1", "major_1_level_2_subject_2", "major_1_level_2_subject_3");
	}
	elseif($sheet_index == 7)
	{
		$field_keys = array("major_1_level_3_subject_1", "major_1_level_3_subject_1", "major_1_level_3_subject_1");
	}
	elseif($sheet_index == 8)
	{
		// Major
	}
	elseif($sheet_index == 9)
	{
		$field_keys = array("major_2_level_1_subject_1", "major_2_level_1_subject_2");
	}
	elseif($sheet_index == 10)
	{
		$field_keys = array("major_2_level_2_subject_1", "major_2_level_2_subject_2", "major_2_level_2_subject_3");
	}
	elseif($sheet_index == 11)
	{
		$field_keys = array("major_2_level_2_subject_3");
	}
	elseif($sheet_index == 12)
	{
		$field_keys = array("breadth_subject_1");
	}
	elseif($sheet_index == 13)
	{
		$field_keys = array("breadth_subject_2", "breadth_subject_3", "breadth_subject_4");
	}
	elseif($sheet_index == 14)
	{
		$field_keys = array("breadth_subject_5", "breadth_subject_6");
	}
	elseif($sheet_index == 15)
	{
		$field_keys = array("half_subject_1", "half_subject_2", "half_subject_3", "half_subject_4", "half_subject_5", "half_subject_6");
	}
	
	
	return $field_keys;
}


function _get_cell_value(&$worksheet_obj, $col_string, $row)
{
	$col = PHPExcel_Cell::columnIndexFromString($col_string) - 1;
	$cell = $worksheet_obj->getCellByColumnAndRow($col, $row);
	$cell_value = $cell->getValue();
	
	return $cell_value;
}


