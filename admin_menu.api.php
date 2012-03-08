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

/**
 * Inform about additional module-specific caches that can be cleared.
 *
 * Administration menu uses this hook to gather information about available
 * caches that can be flushed individually. Each returned item forms a separate
 * menu link below the "Flush all caches" link in the icon menu.
 *
 * @return array
 *   An associative array whose keys denote internal identifiers for a
 *   particular caches (which can be freely defined, but should be in a module's
 *   namespace) and whose values are associative arrays containing:
 *   - title: The name of the cache, without "cache" suffix. This label is
 *     output as link text, but also for the "!title cache cleared."
 *     confirmation message after flushing the cache; make sure it works and
 *     makes sense to users in both locations.
 *   - callback: The name of a function to invoke to flush the individual cache.
 */
function hook_admin_menu_cache_info() {
  $caches['update'] = array(
    'title' => t('Update data'),
    'callback' => '_update_cache_clear',
  );
  return $caches;
}
