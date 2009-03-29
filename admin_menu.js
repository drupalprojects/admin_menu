/* $Id$ */

Drupal.behaviors = Drupal.behaviors || {}; // D5 only.

Drupal.admin = Drupal.admin || { behaviors: {} };

/**
 * Core behavior for Administration menu.
 *
 * Test whether there is an administration menu is in the output and execute all
 * registered behaviors.
 */
Drupal.behaviors.adminMenu = function (context) {
  // Initialize settings.
  Drupal.settings.admin_menu = $.extend({
    margin_top: false,
    position_fixed: false,
    tweak_modules: false,
    tweak_tabs: false,
    destination: ''
  }, Drupal.settings.admin_menu || {});
  var $adminMenu = $('#admin-menu:not(.admin-menu-processed)', context);
  // Apply our behaviors.
  Drupal.admin.attachBehaviors(context, $adminMenu);
};

/**
 * Collapse fieldsets on Modules page.
 *
 * For why multiple selectors see #111719.
 */
Drupal.behaviors.adminMenuCollapseModules = function (context) {
  if (Drupal.settings.admin_menu.tweak_modules) {
    $('#system-modules fieldset:not(.collapsed), #system-modules-1 fieldset:not(.collapsed)', context).addClass('collapsed');
  }
};

/**
 * Apply margin to page.
 *
 * Note that directly applying marginTop does not work in IE. To prevent
 * flickering/jumping page content with client-side caching, this is a regular
 * Drupal behavior.
 */
Drupal.behaviors.adminMenuMarginTop = function (context) {
  if (Drupal.settings.admin_menu.margin_top) {
    $('body', context).addClass('admin-menu');
  }
};

/**
 * @defgroup admin_behaviors Administration behaviors.
 * @{
 */

/**
 * Attach administrative behaviors.
 */
Drupal.admin.attachBehaviors = function (context, $adminMenu) {
  if ($adminMenu.length) {
    $adminMenu.addClass('admin-menu-processed');
    $.each(Drupal.admin.behaviors, function() {
      this(context, $adminMenu);
    });
  }
};

/**
 * Apply 'position: fixed'.
 */
Drupal.admin.behaviors.positionFixed = function (context, $adminMenu) {
  if (Drupal.settings.admin_menu.position_fixed) {
    $adminMenu.css('position', 'fixed');
  }
};

/**
 * Move page tabs into administration menu.
 */
Drupal.admin.behaviors.pageTabs = function (context, $adminMenu) {
  if (Drupal.settings.admin_menu.tweak_tabs) {
    $('ul.tabs.primary li', context).addClass('admin-menu-tab').appendTo('#admin-menu > ul');
    $('ul.tabs.secondary', context).appendTo('#admin-menu > ul > li.admin-menu-tab.active');
    $('ul.tabs.primary', context).remove();
  }
};

/**
 * Inject destination query strings for current page.
 */
Drupal.admin.behaviors.destination = function (context, $adminMenu) {
  if (Drupal.settings.admin_menu.destination) {
    $('.admin-menu-destination', $adminMenu).each(function() {
      this.search += (!this.search.length ? '?' : '&') + Drupal.settings.admin_menu.destination;
    });
  }
}

/**
 * Apply JavaScript-based hovering behaviors.
 *
 * @todo This has to run last.  If another script registers additional behaviors
 *   it will not run last.
 */
Drupal.admin.behaviors.hover = function (context, $adminMenu) {
  // Hover emulation for IE 6.
  if ($.browser.msie && parseInt(jQuery.browser.version) == 6) {
    $('li', $adminMenu).hover(function() {
      $(this).addClass('iehover');
    }, function() {
      $(this).removeClass('iehover');
    });
  }

  // Delayed mouseout.
  $('li', $adminMenu).hover(function() {
    // Stop the timer.
    clearTimeout(this.sfTimer);
    // Display child lists.
    $('> ul', this).css({left: 'auto', display: 'block'})
      // Immediately hide nephew lists.
      .parent().siblings('li').children('ul').css({left: '-999em', display: 'none'});
  }, function() {
    // Start the timer.
    var uls = $('> ul', this);
    this.sfTimer = setTimeout(function() {
      uls.css({left: '-999em', display: 'none'});
    }, 400);
  });
};

/**
 * @} End of "defgroup admin_behaviors".
 */

/**
 * D5 only: Queue our attach behavior.
 */
$(function() {
  Drupal.behaviors.adminMenu(document);
  Drupal.behaviors.adminMenuMarginTop(document);
  Drupal.behaviors.adminMenuCollapseModules(document);
});

