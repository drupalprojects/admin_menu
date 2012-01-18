<?php

/**
 * @file
 * API documentation for Administration menu.
 */

/**
 * Add to the administration menu content before it is rendered.
 *
 * @param array $content
 *   A structured array suitable for drupal_render(), containing:
 *   - menu: The administrative menu of links below the path 'admin/*'.
 *   - icon: The icon menu.
 *   - user: The user items and links.
 *   Passed by reference.
 *
 * @see hook_admin_menu_output_alter()
 * @see admin_menu_links_menu()
 * @see admin_menu_links_icon()
 * @see admin_menu_links_user()
 * @see theme_admin_menu_links()
 */
function hook_admin_menu_output_build(&$content) {
}

/**
 * Change the administration menu content before it is rendered.
 *
 * @param array $content
 *   A structured array suitable for drupal_render(), containing:
 *   - menu: The administrative menu of links below the path 'admin/*'.
 *   - icon: The icon menu.
 *   - user: The user items and links.
 *   Passed by reference.
 *
 * @see hook_admin_menu_output_build()
 * @see admin_menu_links_menu()
 * @see admin_menu_links_icon()
 * @see admin_menu_links_user()
 * @see theme_admin_menu_links()
 */
function hook_admin_menu_output_alter(&$content) {
  // Add new top-level item.
  $content['menu']['myitem'] = array(
    '#title' => t('My item'),
    // #attributes are used for list items (LI). Note the special syntax for
    // the 'class' attribute.
    '#attributes' => array('class' => array('mymodule-myitem')),
    '#href' => 'mymodule/path',
    // #options are passed to l(). Note that you can apply 'attributes' for
    // links (A) here.
    '#options' => array(
      'query' => drupal_get_destination(),
    ),
    // #weight controls the order of links in the resulting item list.
    '#weight' => 50,
  );
  // Add link to manually run cron.
  $content['menu']['myitem']['cron'] = array(
    '#title' => t('Run cron'),
    '#access' => user_access('administer site configuration'),
    '#href' => 'admin/reports/status/run-cron',
  );
}

