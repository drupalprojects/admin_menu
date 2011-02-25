<?php

/**
 * @file
 * Menu build functions for Administration menu.
 */

/**
 * Rebuild administration menu links.
 *
 * This is invoked whenever the menu is rebuilt.
 *
 * @param $_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 * @param $mid_admin
 *   The menu item id to use for the administration menu.
 *
 * @see admin_menu_output()
 */
function admin_menu_build(&$_admin_menu) {
  global $_menu;
  $_admin_menu = array();

  // Get item id of /q=admin, which we suppose to be the root for admin menu.
  $mid_admin = $_menu['path index']['admin'];

  // Fix access permissions for root (category) menu items in Drupal core.
  foreach ($_menu['items'][$mid_admin]['children'] as $pid) {
    // Nothing to do, if item is accessible or contains no children.
    if (!isset($_menu['items'][$pid]['access']) || $_menu['items'][$pid]['access'] != FALSE || !isset($_menu['items'][$pid]['children'])) {
      continue;
    }
    // Make parent accessible if there is any accessible second level child.
    $is_accessible = FALSE;
    foreach ($_menu['items'][$pid]['children'] as $child) {
      if ($_menu['items'][$child]['access'] == TRUE) {
        $is_accessible = TRUE;
      }
    }
    if ($is_accessible) {
      // Since we are overriding system.module's real access values and many
      // modules simply depend on its ill access permissions ('administer site
      // configuration'), we assign not TRUE, but the integer 1 instead, so
      // _admin_menu_get_children() is able to properly inherit access
      // permissions for sub-items that do not define access.
      $_menu['items'][$pid]['access'] = 1;
    }
  }

  // Copy admin menu items into a new menu tree.
  $_admin_menu['index'] = $_menu['path index'];
  $_admin_menu[$mid_admin] = $_menu['items'][$mid_admin];
  admin_menu_item_url($_admin_menu, $mid_admin);
  _admin_menu_get_children($_admin_menu, $_admin_menu[$mid_admin]);

  // Sort items.
  usort($_admin_menu[$mid_admin]['children'], '_admin_menu_sort');

  // Adjust some menu items for better user experience.
  admin_menu_adjust_items($_admin_menu);

  // Allow other modules to integrate with admin_menu.
  foreach (module_implements('admin_menu') as $module) {
    $function = $module . '_admin_menu';
    $function($_admin_menu, TRUE);
  }
}

/**
 * Recursively adds items to the administration menu.
 *
 * Any accessible menu items are added, including local tasks.
 *
 * @param $_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 * @param $item
 *   The (starting) menu item to use for the administration menu.
 */
function _admin_menu_get_children(&$_admin_menu, &$item) {
  global $_menu;

  if (isset($item['children'])) {
    foreach ($item['children'] as $child) {
      // Check access permissions.
      if (!variable_get('admin_menu_show_all', 0)) {
        $item_is_accessible = FALSE;
        if (!isset($_menu['items'][$child]['access'])) {
          // If parent item is one of the admin categories and uses our custom
          // 'access' value defined in admin_menu_build(), then it is not
          // accessible.
          if (isset($item['access']) && $item['access'] === 1) {
            $item_is_accessible = $_menu['items'][$child]['access'] = FALSE;
          }
          // Otherwise, inherit access from parent item.
          elseif (isset($item['access']) && $item['access']) {
            $item_is_accessible = TRUE;
          }
          // Or fallback to regular, advanced access check of the menu system.
          else {
            $item_is_accessible = _menu_item_is_accessible($child);
          }
        }
        else {
          $item_is_accessible = $_menu['items'][$child]['access'];
        }
      }
      else {
        $item_is_accessible = TRUE;
      }
      // Check menu item type.
      $item_is_visible = $_menu['items'][$child]['type'] & (MENU_VISIBLE_IN_TREE | MENU_IS_LOCAL_TASK);

      // Create the child item if it is accessible and visible.
      if ($item_is_accessible && $item_is_visible) {
        $_admin_menu[$child] = $_menu['items'][$child];
        admin_menu_item_url($_admin_menu, $child);

        // Recurse to child menu items.
        if (isset($_menu['items'][$child]['children'])) {
          _admin_menu_get_children($_admin_menu, $_admin_menu[$child]);
          // Sort items.
          usort($_admin_menu[$child]['children'], '_admin_menu_sort');
        }

        // Remove this child item if it is visible in the navigation.
        unset($_menu['visible'][$child]);
      }
      else {
        // Remove child menu item from parent's children list.
        $parent_id = $_menu['items'][$child]['pid'];
        if (isset($_menu['items'][$parent_id]['children']) && is_array($_menu['items'][$parent_id]['children'])) {
          $child_key = array_search($child, $_menu['items'][$parent_id]['children']);
          if ($child_key !== FALSE) {
            unset($_admin_menu[$parent_id]['children'][$child_key]);
          }
        }
      }
    }
  }
}

/**
 * Add some hard-coded features for better user experience.
 *
 * @param &$_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 */
function admin_menu_adjust_items(&$_admin_menu) {
  global $_menu, $user, $base_url;

  // Add the icon containing special links.
  $mid_admin = $_admin_menu['index']['admin'];
  $mid_icon = admin_menu_add_item($_admin_menu, $mid_admin, array(
    'title' => theme('admin_menu_icon'),
    'path' => drupal_get_normal_path(variable_get('site_frontpage', 'node')),
    'weight' => -100,
    'class' => 'admin-menu-icon',
  ));
  $_admin_menu['index']['admin_menu_icon'] = $mid_icon;

  // Add 'administer' item to the icon menu.
  $mid_icon_admin = admin_menu_add_item($_admin_menu, $mid_icon, array('title' => t('Administer'), 'path' => 'admin', 'weight' => 10));
  // ...and reset 'administer' menu item id in path index.
  $_admin_menu['index']['admin'] = $mid_admin;

  // Move 'By module' item if accessible, or remove it.
  $mid_bymodule = $_admin_menu['index']['admin/by-module'];
  if (user_access('administer site configuration')) {
    admin_menu_move_item($_admin_menu, $mid_bymodule, $mid_icon_admin);
  }
  else {
    admin_menu_remove_item($_admin_menu, 'admin/by-module');
  }

  // Remove 'By task' menu item.
  admin_menu_remove_item($_admin_menu, 'admin/by-task');

  // Add system update links.
  if (user_access('administer site configuration')) {
    admin_menu_add_item($_admin_menu, $mid_icon, array('title' => t('Run cron'), 'path' => 'admin/logs/status/run-cron', 'weight' => 50, 'query' => drupal_get_destination()));
  }
  if ($user->uid == 1) {
    admin_menu_add_item($_admin_menu, $mid_icon, array('title' => t('Run updates'), 'path' => $base_url . '/update.php', 'weight' => 50));
  }

  // Add links to drupal.org.
  if (user_access('display drupal links')) {
    $mid_drupal = admin_menu_add_item($_admin_menu, $mid_icon, array('title' => t('Drupal.org'), 'path' => 'http://drupal.org', 'weight' => 100));
    admin_menu_add_item($_admin_menu, $mid_drupal, array('title' => t('Drupal issue queue'), 'path' => 'http://drupal.org/project/issues/drupal'));

    // Add links to project issue queues.
    $projects = array();
    foreach (module_list(FALSE, FALSE, TRUE) as $module) {
      $info = _module_parse_info_file(drupal_get_path('module', $module) . '/' . $module . '.info');
      if (!isset($info['project']) || (isset($info['project']) && ($info['project'] == 'drupal' || isset($projects[$info['project']])))) {
        continue;
      }
      $projects[$info['project']] = 1;
      $url = 'http://drupal.org/project/issues/' . $info['project'];
      // Filtering project versions via query string is not yet supported.
      // @see http://drupal.org/node/97569
      // $url .= !empty($info['version']) ? '/'. $info['version'] : '';
      admin_menu_add_item($_admin_menu, $mid_drupal, array('title' => t('@title issue queue', array('@title' => $info['name'])), 'path' => $url));
    }
  }

  // Add logout item.
  if ($user->uid > 0) {
    $mid_user = $_menu['path index']["user/$user->uid"];
    admin_menu_add_item($_admin_menu, $mid_admin, array(
      'title' => t('@name', array('@name' => $user->name)),
      'path' => $_menu['items'][$mid_user]['path'],
      'weight' => -99,
      'class' => 'admin-menu-action admin-menu-account',
    ), $mid_user);
    $mid_logout = $_menu['path index']['logout'];
    admin_menu_add_item($_admin_menu, $mid_admin, array(
      'title' => t('Logout'),
      'path' => $_menu['items'][$mid_logout]['path'],
      'weight' => -100,
      'class' => 'admin-menu-action',
    ), $mid_logout);
  }

  // Add 'Create <content_type>' menu items to Content management > Content.
  // If an user does not have 'administer nodes' permission, we need to copy
  // the 'Create content' item from the Navigation menu into 'Content
  // management' and append node/add/* items there instead.
  $mid_node_add = $_menu['path index']['node/add'];
  if (!user_access('administer nodes')) {
    $mid_content_management = $_admin_menu['index']['admin/content'];
    $create_content = $_menu['items'][$mid_node_add];
    $create_content['weight'] = 0;
    $mid_content = admin_menu_add_item($_admin_menu, $mid_content_management, $create_content, $mid_node_add);
  }
  else {
    $mid_content = $_admin_menu['index']['admin/content/node'];
  }
  admin_menu_copy_items($_admin_menu, $mid_node_add, $mid_content, t('Add !title'));

  // Add 'Edit <content_type>' menu items to Content management > Content types.
  if (module_exists('content') && user_access('administer content types')) {
    $mid_content_types = $_admin_menu['index']['admin/content/types'];
    $types = node_get_types();
    $names = node_get_types('names');
    foreach ($names as $key => $name) {
      $type = $types[$key];
      if (function_exists($type->module . '_form')) {
        admin_menu_add_item($_admin_menu, $mid_content_types, array(
          'title' => t('Edit !name', array('!name' => $name)),
          'path' => 'admin/content/types/' . str_replace('_', '-', $type->type) . '/fields',
          'weight' => -5,
        ));
      }
    }
  }

  // Add 'Edit <view>' menu items to Site building > Views.
  if (module_exists('views') && user_access('administer views')) {
    $mid_views_list = $_admin_menu['index']['admin/build/views/list'];
    $_admin_menu[$mid_views_list]['weight'] = -10;
    $mid_views = $_admin_menu['index']['admin/build/views'];
    $result = db_query("SELECT name FROM {view_view} ORDER BY name");
    while ($view = db_fetch_object($result)) {
      admin_menu_add_item($_admin_menu, $mid_views, array(
        'title' => t('Edit !name', array('!name' => $view->name)),
        'path' => "admin/build/views/$view->name/edit",
        'weight' => -5,
      ));
    }
  }

  // Add 'Edit <panel>' menu items to Panels.
  $modules = array(
    'panels_page' => array(
      'path' => 'admin/panels/panel-page/!name/edit/content',
      'parent' => 'admin/panels/panel-page',
      'perm' => 'create panel-pages',
    ),
    'panels_mini' => array(
      'path' => 'admin/panels/panel-mini/!name/edit/content',
      'parent' => 'admin/panels/panel-mini',
      'perm' => 'create mini panels',
    ),
    'panels_views' => array(
      'path' => 'admin/panels/views/edit/!name',
      'parent' => 'admin/panels/views',
      'perm' => 'administer panel views',
    ),
  );
  foreach ($modules as $module => $attribs) {
    if (user_access($attribs['perm'])) {
      $mid_panels = $_admin_menu['index'][$attribs['parent']];
      foreach ((array)module_invoke($module, 'load_all') as $panel) {
        if (empty($panel->disabled)) {
          admin_menu_add_item($_admin_menu, $mid_panels, array(
            'title' => t('Edit !name', array('!name' => $panel->name)),
            'path' => strtr($attribs['path'], array('!name' => $panel->name)),
            'weight' => -5,
          ));
        }
      }
    }
  }
  // Add developer modules toggle link.
  if (user_access('administer site configuration')) {
    $mid_icon = $_admin_menu['index']['admin_menu_icon'];
    $current_state = variable_get('admin_menu_devel_modules_enabled', NULL);
    admin_menu_add_item($_admin_menu, $mid_icon, array(
      'title' => isset($current_state) ? t('Enable developer modules') : t('Disable developer modules'),
      'path' => 'admin_menu/toggle-modules',
      'weight' => 88,
    ));
  }
}

/**
 * Recursively copy menu items from a source parent menu item to a target item.
 *
 * @param array $_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 * @param int $source_pid
 *   A source parent menu item id from which children shall be copied.
 * @param int $target_pid
 *   A target parent menu item id.
 * @param string $title
 *   An optional string containing the token !title, that has already been
 *   passed through t(), which will be used to dynamically replace previous
 *   menu item titles.
 * @param bool $tree
 *   Whether to rebuild the complete hierarchy from the source parent menu item
 *   or copy menu items flattened. Defaults to TRUE.
 */
function admin_menu_copy_items(&$_admin_menu, $source_pid, $target_pid, $title = NULL, $tree = TRUE) {
  global $_menu;

  if (isset($_menu['items'][$source_pid]['children']) && isset($_admin_menu[$target_pid])) {
    foreach ($_menu['items'][$source_pid]['children'] as $mid) {
      $item = $_menu['items'][$mid];
      if (!$item['access']) {
        continue;
      }
      if (isset($title)) {
        $item['title'] = check_plain(strtr($title, array('!title' => $_menu['items'][$mid]['title'])));
      }
      // Only add child to target if it does not already exist.
      if (!in_array($mid, $_admin_menu[$target_pid]['children'])) {
        admin_menu_add_item($_admin_menu, $target_pid, $item, $mid);
      }

      // Recurse into children.
      if (isset($_menu['items'][$mid]['children']) && count($_menu['items'][$mid]['children'])) {
        if ($tree) {
          admin_menu_copy_items($_admin_menu, $mid, $mid, $title);
        }
        else {
          admin_menu_copy_items($_admin_menu, $mid, $target_pid, $title, FALSE);
          unset($_admin_menu[$mid]['children']);
          // Note: Uncomment following lines to remove unnecessary parent items.
          // unset($_admin_menu[$target_pid]['children'][array_search($mid, $_admin_menu[$target_pid]['children'])]);
          // unset($_admin_menu[$mid]);
        }
      }
    }
  }
}

/**
 * Moves the child pointer of a menu item to a new parent.
 *
 * @param array $_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 * @param int $mid
 *   The menu item id of the item to move.
 * @param int $pid
 *   The menu item id of the new parent.
 */
function admin_menu_move_item(&$_admin_menu, $mid, $pid) {
  global $_menu;

  if (isset($_admin_menu[$mid]) && isset($_admin_menu[$pid])) {
    // Remove current child pointer. Determine the path from the index
    // because it contains the unprocessed urls.
    $index = array_flip($_admin_menu['index']);
    admin_menu_remove_item($_admin_menu, $index[$mid]);

    // Insert new child pointer.
    $_admin_menu[$mid]['pid'] = $pid;
    $_admin_menu[$pid]['children'][] = $mid;

    // Sort items.
    usort($_admin_menu[$pid]['children'], '_admin_menu_sort');
    return TRUE;
  }
  return FALSE;
}

/**
 * Removes the child pointer for a menu item.
 *
 * @param array $_admin_menu
 *   An array containing the complete administration menu structure, passed by
 *   reference.
 * @param string $path
 *   The menu path to remove, e.g. 'foo/bar'.
 */
function admin_menu_remove_item(&$_admin_menu, $path) {
  global $_menu;

  $mid = $_admin_menu['index'][$path];
  if (isset($_admin_menu[$mid])) {
    $pid = $_admin_menu[$mid]['pid'];
    $child_key = array_search($mid, $_admin_menu[$pid]['children']);
    if ($child_key !== FALSE) {
      unset($_admin_menu[$pid]['children'][$child_key]);
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Form builder function for module settings.
 */
function admin_menu_theme_settings() {
  $form['admin_menu_margin_top'] = array(
    '#type' => 'checkbox',
    '#title' => t('Adjust top margin'),
    '#default_value' => variable_get('admin_menu_margin_top', 1),
    '#description' => t('If enabled, the site output is shifted down approximately 20 pixels from the top of the viewport to display the administration menu. If disabled, some absolute- or fixed-positioned page elements may be covered by the administration menu.'),
  );
  $form['admin_menu_position_fixed'] = array(
    '#type' => 'checkbox',
    '#title' => t('Keep menu at top of page'),
    '#default_value' => variable_get('admin_menu_position_fixed', 0),
    '#description' => t('If enabled, the administration menu is always displayed at the top of the browser viewport (even after the page is scrolled). <strong>Note: In some browsers, this setting results in a malformed page, an invisible cursor, non-selectable elements in forms, or other issues. Disable this option if these issues occur.</strong>'),
  );
  $form['tweaks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced settings'),
  );
  $form['tweaks']['admin_menu_tweak_menu'] = array(
    '#type' => 'checkbox',
    '#title' => t('Collapse menus in menu administration'),
    '#default_value' => variable_get('admin_menu_tweak_menu', 0),
    '#description' => t('If enabled, menu containers on the <a href="!menu-url">Site building &raquo; Menus</a> page will be collapsed like fieldsets.', array('!menu-url' => url('admin/build/menu'))),
  );
  $form['tweaks']['admin_menu_tweak_modules'] = array(
    '#type' => 'checkbox',
    '#title' => t('Collapse fieldsets on modules page'),
    '#default_value' => variable_get('admin_menu_tweak_modules', 0),
    '#description' => t('If enabled, fieldsets on the <a href="!modules-url">modules</a> page are automatically collapsed when loading the page.', array('!modules-url' => url('admin/build/modules'))),
  );
  if (module_exists('util')) {
    $form['tweaks']['admin_menu_tweak_modules']['#description'] .= '<br /><strong>' . t('If the Utility module was installed for this purpose, it can be safely disabled and uninstalled.') . '</strong>';
  }
  $form['tweaks']['admin_menu_tweak_tabs'] = array(
    '#type' => 'checkbox',
    '#title' => t('Move local tasks into menu'),
    '#default_value' => variable_get('admin_menu_tweak_tabs', 0),
    '#description' => t('If enabled, the tabs on the current page are moved into the administration menu. This feature is only available in themes that use the CSS classes <code>tabs primary</code> and <code>tabs secondary</code> for tabs.'),
  );

  // Fetch all available modules manually, since module_list() only returns
  // currently enabled modules, which makes this setting pointless if developer
  // modules are currently disabled.
  $all_modules = array();
  $result = db_query("SELECT name, filename FROM {system} WHERE type = 'module' ORDER BY name ASC");
  while ($module = db_fetch_object($result)) {
    if (file_exists($module->filename)) {
      $all_modules[$module->name] = $module->name;
    }
  }
  $devel_modules = variable_get('admin_menu_devel_modules', _admin_menu_developer_modules());
  $devel_modules = array_intersect($all_modules, $devel_modules);
  $form['tweaks']['admin_menu_devel_modules_skip'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Developer modules to keep enabled'),
    '#default_value' => variable_get('admin_menu_devel_modules_skip', array()),
    '#options' => $devel_modules,
    '#access' => !empty($devel_modules),
    '#description' => t('Enabled developer modules in this list will not be disabled when the link <em>Disable developer modules</em> in the menu below the icon is invoked.'),
  );

  return system_settings_form($form);
}

/**
 * Implementation of hook_form_alter().
 */
function admin_menu_form_alter($form_id, &$form) {
  if ($form_id == 'devel_admin_settings') {
    admin_menu_form_devel_admin_settings_alter($form);
  }
  elseif ($form_id == 'system_clean_url_settings') {
    admin_menu_form_system_clean_url_settings_alter($form);
  }
}

/**
 * Implementation of hook_form_FORM_ID_alter().
 */
function admin_menu_form_system_clean_url_settings_alter(&$form) {
  $form['#submit']['admin_menu_system_clean_url_settings_form_submit'] = array();
}

/**
 * Form submit handler to flush client-side cache hashes when clean URLs are toggled.
 */
function admin_menu_system_clean_url_settings_form_submit($form_id, $form_values) {
  // Flush cached output of admin_menu.
  cache_clear_all('admin_menu:', 'cache_menu', TRUE);
  // Flush client-side cache hashes.
  cache_clear_all('*', 'cache_admin_menu', TRUE);
}

/**
 * Implementation of hook_form_FORM_ID_alter().
 *
 * Extends Devel module with Administration menu developer settings.
 */
function admin_menu_form_devel_admin_settings_alter(&$form) {
  // Shift system_settings_form buttons.
  $weight = isset($form['buttons']['#weight']) ? $form['buttons']['#weight'] : 0;
  $form['buttons']['#weight'] = $weight + 1;

  $form['admin_menu'] = array(
    '#type' => 'fieldset',
    '#title' => t('Administration menu settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $display_options = array('mid', 'weight', 'pid');
  $display_options = array(0 => t('None'), 'mid' => t('Menu link ID'), 'weight' => t('Weight'), 'pid' => t('Parent link ID'));
  $form['admin_menu']['admin_menu_display'] = array(
    '#type' => 'radios',
    '#title' => t('Display additional data for each menu item'),
    '#default_value' => variable_get('admin_menu_display', 0),
    '#options' => $display_options,
    '#description' => t('Display the selected items next to each menu item link.'),
  );
  $form['admin_menu']['admin_menu_show_all'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display all menu items'),
    '#default_value' => variable_get('admin_menu_show_all', 0),
    '#description' => t('If enabled, all menu items are displayed regardless of your site permissions. <em>Note: Do not enable on a production site.</em>'),
  );
}

/**
 * Menu callback; Enable/disable developer modules.
 *
 * This can save up to 150ms on each uncached page request.
 */
function _admin_menu_toggle_modules() {
  $rebuild = FALSE;
  $saved_state = variable_get('admin_menu_devel_modules_enabled', NULL);
  if (isset($saved_state)) {
    // Re-enable modules that were enabled before.
    module_enable($saved_state);
    variable_del('admin_menu_devel_modules_enabled');
    drupal_set_message(t('Enabled these modules: !module-list.', array('!module-list' => implode(', ', $saved_state))));
    $rebuild = TRUE;
  }
  else {
    // Allow site admins to override this variable via settings.php.
    $devel_modules = variable_get('admin_menu_devel_modules', _admin_menu_developer_modules());
    // Store currently enabled modules in a variable.
    $devel_modules = array_intersect(module_list(FALSE, FALSE), $devel_modules);
    $devel_modules = array_diff($devel_modules, variable_get('admin_menu_devel_modules_skip', array()));
    if (!empty($devel_modules)) {
      variable_set('admin_menu_devel_modules_enabled', $devel_modules);
      // Disable developer modules.
      module_disable($devel_modules);
      drupal_set_message(t('Disabled these modules: !module-list.', array('!module-list' => implode(', ', $devel_modules))));
      $rebuild = TRUE;
    }
    else {
      drupal_set_message(t('No developer modules are enabled.'));
    }
  }
  if ($rebuild) {
    // Make sure everything is rebuilt, basically a combination of the calls
    // from system_modules() and system_modules_submit().
    menu_rebuild();
    cache_clear_all();
    drupal_clear_css_cache();
    // Finally, flush admin_menu's cache.
    admin_menu_flush_caches();
  }
  drupal_goto(referer_uri());
}

/**
 * Helper function to return a default list of developer modules.
 */
function _admin_menu_developer_modules() {
  return array(
    'cache_disable',
    'coder',
    'content_copy',
    'debug',
    'delete_all',
    'demo',
    'devel',
    'devel_node_access',
    'devel_themer',
    'macro',
    'form_controller',
    'imagecache_ui',
    'journal',
    'rules_admin',
    'trace',
    'upgrade_status',
    'user_display_ui',
    'util',
    'views_ui',
    'views_theme_wizard',
  );
}

/**
 * Render an icon to display in the administration menu.
 *
 * @ingroup themeable
 */
function theme_admin_menu_icon() {
  return '<img class="admin-menu-icon" src="' . (theme_get_setting('toggle_favicon') ? theme_get_setting('favicon') : base_path() . 'misc/favicon.ico') . '" width="16" height="16" alt="' . t('Home') . '" />';
}

