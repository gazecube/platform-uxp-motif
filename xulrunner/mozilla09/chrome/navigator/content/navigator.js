/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Blake Ross <blakeross@telocity.com>
 *   Peter Annema <disttsc@bart.nl>
 */

const nsIWebNavigation = Components.interfaces.nsIWebNavigation;

var gURLBar = null;
var gNavigatorBundle;
var gBrandBundle;
var gNavigatorRegionBundle;
var gBrandRegionBundle;

var pref = Components.classes["@mozilla.org/preferences;1"]
                     .getService(Components.interfaces.nsIPref);

var appCore = null;

//cached elements
var gBrowser = null;

// focused frame URL
var gFocusedURL = null;


/**
* We can avoid adding multiple load event listeners and save some time by adding
* one listener that calls all real handlers.
*/

function loadEventHandlers(event)
{
  // Filter out events that are not about the document load we are interested in
  if (event.target == _content.document) {
    UpdateBookmarksLastVisitedDate(event);
    UpdateInternetSearchResults(event);
    checkForDirectoryListing();
    getContentAreaFrameCount();
    postURLToNativeWidget();
  }
}

/**
 * Determine whether or not the content area is displaying a page with frames,
 * and if so, toggle the display of the 'save frame as' menu item.
 **/
function getContentAreaFrameCount()
{
  var saveFrameItem = document.getElementById("savepage");
  if (!_content.frames.length || !isDocumentFrame(document.commandDispatcher.focusedWindow))
    saveFrameItem.setAttribute("hidden", "true");
  else
    saveFrameItem.removeAttribute("hidden");
}

// When a content area frame is focused, update the focused frame URL
function contentAreaFrameFocus()
{
  var focusedWindow = document.commandDispatcher.focusedWindow;
  if (isDocumentFrame(focusedWindow)) {
    gFocusedURL = focusedWindow.location.href;
    var saveFrameItem = document.getElementById("savepage");
    saveFrameItem.removeAttribute("hidden");
  }
}

//////////////////////////////// BOOKMARKS ////////////////////////////////////

function UpdateBookmarksLastVisitedDate(event)
{
  // XXX This somehow causes a big leak, back to the old way
  //     till we figure out why. See bug 61886.
  // var url = getWebNavigation().currentURI.spec;
  var url = _content.location.href;
  if (url) {
    // if the URL is bookmarked, update its "Last Visited" date
    var bmks = Components.classes["@mozilla.org/browser/bookmarks-service;1"]
                         .getService(Components.interfaces.nsIBookmarksService);

    bmks.UpdateBookmarkLastVisitedDate(url, _content.document.characterSet);
  }
}

function UpdateInternetSearchResults(event)
{
  // XXX This somehow causes a big leak, back to the old way
  //     till we figure out why. See bug 61886.
  // var url = getWebNavigation().currentURI.spec;
  var url = _content.location.href;
  if (url) {
    try {
      var search = Components.classes["@mozilla.org/rdf/datasource;1?name=internetsearch"]
                             .getService(Components.interfaces.nsIInternetSearchService);

      var searchInProgressFlag = search.FindInternetSearchResults(url);

      if (searchInProgressFlag) {
        var autoOpenSearchPanel = pref.GetBoolPref("browser.search.opensidebarsearchpanel");

        if (autoOpenSearchPanel)
          RevealSearchPanel();
      }
    } catch (ex) {
    }
  }
}

function getBrowser()
{
  if (!gBrowser)
    gBrowser = document.getElementById("content");
  return gBrowser;
}

function getWebNavigation()
{
  try {
    return getBrowser().webNavigation;
  } catch (e) {
    return null;
  }
}

function getMarkupDocumentViewer()
{
  return getBrowser().markupDocumentViewer;
}

function getHomePage()
{
  var url;
  try {
    url = pref.getLocalizedUnicharPref("browser.startup.homepage");
  } catch (e) {
  }

  // use this if we can't find the pref
  if (!url)
    url = gNavigatorRegionBundle.getString("homePageDefault");

  return url;
}

function UpdateBackForwardButtons()
{
  var backBroadcaster = document.getElementById("canGoBack");
  var forwardBroadcaster = document.getElementById("canGoForward");
  var webNavigation = getWebNavigation();

  backBroadcaster.setAttribute("disabled", !webNavigation.canGoBack);
  forwardBroadcaster.setAttribute("disabled", !webNavigation.canGoForward);
}


function nsButtonPrefListener()
{
  try {
    pref.addObserver(this.domain, this);
  } catch(ex) {
    dump("Failed to observe prefs: " + ex + "\n");
  }
}

// implements nsIObserver
nsButtonPrefListener.prototype =
{
  domain: "browser.toolbars.showbutton",
  Observe: function(subject, topic, prefName)
  {
    // verify that we're changing a button pref
    if (topic != "nsPref:changed") return;
    if (prefName.substr(0, this.domain.length) != this.domain) return;

    var buttonName = prefName.substr(this.domain.length+1);
    var buttonId = buttonName + "-button";
    var button = document.getElementById(buttonId);

    var show = pref.GetBoolPref(prefName);
    if (show)
      button.setAttribute("hidden","false");
    else
      button.setAttribute("hidden", "true");
  }
}

function Startup()
{
  // init globals
  gNavigatorBundle = document.getElementById("bundle_navigator");
  gBrandBundle = document.getElementById("bundle_brand");
  gNavigatorRegionBundle = document.getElementById("bundle_navigator_region");
  gBrandRegionBundle = document.getElementById("bundle_brand_region");

  gBrowser = document.getElementById("content");
  gURLBar = document.getElementById("urlbar");

  var webNavigation;
  try {
    // Create the browser instance component.
    appCore = Components.classes["@mozilla.org/appshell/component/browser/instance;1"]
                        .createInstance(Components.interfaces.nsIBrowserInstance);
    if (!appCore)
      throw Components.results.NS_ERROR_FAILURE;

    webNavigation = getWebNavigation();
    if (!webNavigation)
      throw Components.results.NS_ERROR_FAILURE;
  } catch (e) {
    alert("Error creating browser instance");
    window.close(); // Give up.
    return;
  }

  // Do all UI building here:

  setOfflineStatus();

  // set home button tooltip text
  var homePage = getHomePage();
  if (homePage)
    document.getElementById("home-button").setAttribute("tooltiptext", homePage);

  try {
    var searchMode = pref.GetIntPref("browser.search.mode");
    setBrowserSearchMode(searchMode);
  } catch (ex) {
  }

  // initialize observers and listeners
  window.XULBrowserWindow = new nsBrowserStatusHandler();
  window.buttonPrefListener = new nsButtonPrefListener();

  // XXXjag hack for directory.xul/js
  _content.appCore = appCore;

  // Initialize browser instance..
  appCore.setWebShellWindow(window);

  // Add a capturing event listener to the content area
  // (rjc note: not the entire window, otherwise we'll get sidebar pane loads too!)
  // getBrowser().addEventListener("load", loadEventHandler, true);
  getBrowser().addEventListener("load", loadEventHandler, true);

  // Add content area focus listener
  getBrowser().addEventListener("focus", contentAreaFrameFocus, true);

  // Set the default home page if necessary.
  if (!_content.location.href || _content.location.href == "about:blank") {
    var url = getHomePage();
    if (url)
      loadURI(url);
  }

  // Set the visible toolbars.
  SetUpToolbars();

  // Set up the personal toolbar.
  personalToolbarObserver.init();

  // Set up the window status bar.
  var commandDispatcher = document.commandDispatcher;
  commandDispatcher.addCommandUpdater(this);

  // Update back/forward buttons.
  UpdateBackForwardButtons();

  // Load up the default taskbar and offline status.
  ToggleTaskbar();

  // Set up auto complete on the URL bar.
  try {
    gURLBar.session = Components.classes["@mozilla.org/autocomplete/session;type=history",1].createInstance();
  } catch (ex) {
  }

  checkForDefaultBrowser();

  // Start the tooltip service.
  fillThisInTooltipService();

  // XXX This command dispatcher way is kunk.
  //     These commands should all be smart enough to disable themselves.
  goEditEditabldIspatcher();

  // Notify sidebar of the current page.
  UpdateSidebar(true);

  // Listen for theme pref changes.
  var observerService = Components.classes["@mozilla.org/observer-service;1"]
                                    .getService(Components.interfaces.nsIObserverService);
  observerService.addObserver(this, "locale-options-changed", false);
  observerService.addObserver(this, "theme-changed", false);

  // Notify sidebar of the current page.
  UpdateSidebar(true);

  // Add the console listener.
  /* XXX We have no good way of attaching listeners to XUL elements when
     they're created, so this gets commented out for now.
  var consoleService = Components.classes["@mozilla.org/consoleservice;1"]
                                 .getService(Components.interfaces.nsIConsoleService);

  if (consoleService)
    consoleService.registerListener(consoleListener);
  */
}

function loadErrorConsole(aEvent)
{
  if (aEvent.detail == 2)
    toJavaScriptConsole();
}

function clearErrorNotification()
{
  var statusbarDisplay = document.getElementById("statusbar-display");
  statusbarDisplay.removeAttribute("error");
  statusbarDisplay.removeEventListener("click", loadErrorConsole, true);
  consoleListener.isShowingError = false;
}

var urlWidgetService = null;
try {
  urlWidgetService = Components.classes["@mozilla.org/urlwidget;1"]
                               .getService(Components.interfaces.nsIUrlWidget);
} catch (ex) {
}

//Posts the currently displayed url to a native widget so third-party apps can observe it.
function postURLToNativeWidget()
{
  if (urlWidgetService) {
    // XXX This somehow causes a big leak, back to the old way
    //     till we figure out why. See bug 61886.
    // var url = getWebNavigation().currentURI.spec;
    var url = _content.location.href;
    try {
      urlWidgetService.SetURLToHiddenControl(url, window);
    } catch(ex) {
    }
  }
}

function checkForDirectoryListing()
{
  if ( "HTTPIndex" in _content &&
       _content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
    _content.defaultCharacterset = getMarkupDocumentViewer().defaultCharacterSet;
    _content.parentWindow = window;
  }
}

/**
 * Content area tooltip.
 * XXX  - this must move into XBL binding/equiv! Do not want to pollute
 *       navigator.js with functionality that can be encapsulated into
 *       browser widget. TEMPORARY!
 *
 * NOTE: Any changes to this routine need to be mirrored in ChromeListener::FindTitleText()
 *       (located in mozilla/embedding/browser/webBrowser/nsDocShellTreeOwner.cpp)
 *       which performs the same function, but for embedded clients that
 *       don't use a XUL/JS layer. It is important that the logic of
 *       these two routines be kept more or less in sync.
 *       (pinkerton)
 **/
function FillInHTMLTooltip(tipElement)
{
  const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
  const XLinkNS = "http://www.w3.org/1999/xlink";
  const Node = { ELEMENT_NODE : 1 }; // XXX Components.interfaces.Node;

  var retVal = false;
  var tipNode = document.getElementById("HTML_TOOLTIP_tooltipBox");
  try {
    while (tipNode.hasChildNodes())
      tipNode.removeChild(tipNode.firstChild);

    var titleText = "";
    var XLinkTitleText = "";

    while (!titleText && !XLinkTitleText && tipElement) {
      if (tipElement.nodeType == Node.ELEMENT_NODE) {
        titleText = tipElement.getAttribute("title");
        XLinkTitleText = tipElement.getAttributeNS(XLinkNS, "title");
      }
      tipElement = tipElement.parentNode;
    }

    var texts = [titleText, XLinkTitleText];

    for (var i = 0; i < texts.length; ++i) {
      var t = texts[i];
      if (t.search(/\S/) >= 0) {
        var tipLineElem = tipNode.ownerDocument.createElementNS(XULNS, "text");
        tipLineElem.setAttribute("value", t);
        tipNode.appendChild(tipLineElem);

        retVal = true;
      }
    }
  } catch (e) {
  }

  return retVal;
}

/**
 * Use Stylesheet functions.
 *     Written by Tim Hill (bug 6782)
 **/
function stylesheetFillPopup(forDocument, menuPopup, itemNoOptStyles)
{
  var children = menuPopup.childNodes;
  var i;
  for (i = 0; i < children.length; ++i) {
    var child = children[i];
    if (child.getAttribute("class") == "authssmenuitem")
      menuPopup.removeChild(child);
  }

  var noOptionalStyles = true;
  var styleSheets = forDocument.styleSheets;
  var currentStyleSheets = [];

  for (i = 0; i < styleSheets.length; ++i) {
    var currentStyleSheet = styleSheets[i];

    if (currentStyleSheet.title) {
      if (!currentStyleSheet.disabled)
        noOptionalStyles = false;

      var lastWithSameTitle = null;
      if (currentStyleSheet.title in currentStyleSheets)
        lastWithSameTitle = currentStyleSheets[currentStyleSheet.title];

      if (!lastWithSameTitle) {
        var menuItem = document.createElement("menuitem");
        menuItem.setAttribute("label", currentStyleSheet.title);
        menuItem.setAttribute("data", currentStyleSheet.title);
        menuItem.setAttribute("type", "radio");
        menuItem.setAttribute("checked", !currentStyleSheet.disabled);
        menuItem.setAttribute("class", "authssmenuitem");
        menuItem.setAttribute("name", "authorstyle");
        menuItem.setAttribute("oncommand", "stylesheetSwitch(_content.document, this.getAttribute('data'))");
        menuPopup.appendChild(menuItem);
        currentStyleSheets[currentStyleSheet.title] = menuItem;
      } else {
        if (currentStyleSheet.disabled)
          lastWithSameTitle.setAttribute("checked", false);
      }
    }
  }
  itemNoOptStyles.setAttribute("checked", noOptionalStyles);
}

function stylesheetSwitch(forDocument, title)
{
  var docStyleSheets = forDocument.styleSheets;

  for (var i = 0; i < docStyleSheets.length; ++i) {
    var docStyleSheet = docStyleSheets[i];

    if (docStyleSheet.title)
      docStyleSheet.disabled = (docStyleSheet.title != title);
    else if (docStyleSheet.disabled)
      docStyleSheet.disabled = false;
  }
}

function applyTheme(themeName)
{
  var chromeRegistry = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                                 .getService(Components.interfaces.nsIChromeRegistry);

  chromeRegistry.selectSkin(themeName.getAttribute("name"), true);
  chromeRegistry.refreshSkins();
}

function getNewThemes()
{
  loadURI(gBrandRegionBundle.getString("getNewThemesURL"));
}

function URLBarLeftClickHandler(aEvent)
{
  if (pref.GetBoolPref("browser.urlbar.clickSelectsAll")) {
    var URLBar = aEvent.target;
    URLBar.setSelectionRange(0, URLBar.value.length);
  }
}

function URLBarBlurHandler(aEvent)
{
  if (pref.GetBoolPref("browser.urlbar.clickSelectsAll")) {
    var URLBar = aEvent.target;
    URLBar.setSelectionRange(0, 0);
  }
}

// This function gets the "windows hooks" service and has it check its setting
// This will do nothing on platforms other than Windows.
function checkForDefaultBrowser()
{
  try {
    Components.classes["@mozilla.org/winhooks;1"]
              .getService(Components.interfaces.nsIWindowsHooks)
              .checkSettings(window);
  } catch(e) {
  }
}

function ShowAndSelectContentsOfURLBar()
{
  var navBar = document.getElementById("nav-bar");
  
  // If it's hidden, show it.
  if (navBar.getAttribute("hidden") == "true")
    goToggleToolbar('nav-bar','cmd_viewnavbar');
 
  if (gURLBar.value)
    gURLBar.select();
  else
    gURLBar.focus();
}

// If "ESC" is pressed in the url bar, we replace the urlbar's value with the url of the page
// and highlight it, unless it is about:blank, where we reset it to "".
function resetURLBar()
{
  var url = _content.location.href;
  var throbberElement = document.getElementById("navigator-throbber");

  if (!throbberElement.getAttribute("busy")){
    if (url != "about:blank"){ 
      gURLBar.value = url;
      gURLBar.select();
    } else { //if about:blank, urlbar becomes ""
      gURLBar.value = "";
    }
  }
}

function handleURLBarKeyPress(event)
{
  if (event.keyCode == KeyEvent.DOM_VK_RETURN) { addToUrlbarHistory(); BrowserLoadURL(); } 
  else if (event.keyCode == KeyEvent.DOM_VK_ESCAPE) { resetURLBar(); }
}
