/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const { Cc, Ci, Cu } = require("chrome");
const Services = require("Services");
const gcli = require("gcli/index");
const { DebuggerServer } = require("resource://gre/modules/devtools/dbg-server.jsm");

const BRAND_SHORT_NAME = Cc["@mozilla.org/intl/stringbundle;1"]
                           .getService(Ci.nsIStringBundleService)
                           .createBundle("chrome://branding/locale/brand.properties")
                           .GetStringFromName("brandShortName");

exports.items = [
  {
    name: "listen",
    description: gcli.lookup("listenDesc"),
    manual: gcli.lookupFormat("listenManual2", [ BRAND_SHORT_NAME ]),
    params: [
      {
        name: "port",
        type: "number",
        get defaultValue() {
          return Services.prefs.getIntPref("devtools.debugger.chrome-debugging-port");
        },
        description: gcli.lookup("listenPortDesc"),
      }
    ],
    exec: function(args, context) {
      Cc["@mozilla.org/devtools/DebuggerServerController;1"]
        .getService(Ci.nsIDebuggerServerController).start(args.port);

      if (!DebuggerServer._listener) {
        return gcli.lookup("listenFailed");
      }

      return gcli.lookupFormat("listenInitOutput", [ '' + args.port ]);
    },
  }
];
