// Workaround for Chrome 48 getTransformToElement removal
// https://github.com/clientIO/joint/issues/203
SVGElement.prototype.getTransformToElement = SVGElement.prototype.getTransformToElement || function(toElement) {
    return toElement.getScreenCTM().inverse().multiply(this.getScreenCTM());
};

// Basic Block Graph 
var BBGraph = function () {
  this.vertices = {};
  this.edges = [];
  this.elements = [];
  this.links = [];
  this.fcn_offset = 0;

  joint.shapes.html = {};
  joint.shapes.html.Element = joint.shapes.basic.Rect.extend({
      defaults: joint.util.deepSupplement({
          type: 'html.Element',
          attrs: {
              rect: { stroke: r2ui.colors[".ec_gui_border"], fill: r2ui.colors[".ec_gui_alt_background"] }
          }
      }, joint.shapes.basic.Rect.prototype.defaults)
  });

  joint.shapes.html.ElementView = joint.dia.ElementView.extend({
      initialize: function() {
          _.bindAll(this, 'updateBox');
          joint.dia.ElementView.prototype.initialize.apply(this, arguments);
          this.$box = $(_.template(this.model.get('html'))());
          this.$box.find('input').on('mousedown click', function(evt) { evt.stopPropagation(); });
          this.model.on('change', this.updateBox, this);
          this.updateBox();
      },
      render: function() {
          joint.dia.ElementView.prototype.render.apply(this, arguments);
          this.paper.$el.prepend(this.$box);
          this.updateBox();
          return this;
      },
      updateBox: function(event) {
        // move the html mask when moving the svg rect
        var bbox = this.model.getBBox();
        this.$box.css({ width: bbox.width + 2, height: bbox.height, left: bbox.x - 1, top: bbox.y + 7});
      }
  });
};

BBGraph.prototype.addVertex = function(addr, vlen, dom) {
  if (this.vertices[addr] === undefined) {
    this.vertices[addr] = {};
    this.vertices[addr].parents = [];
    this.vertices[addr].children = [];
    if (vlen === undefined) {
      this.vertices[addr].len = 1;
      dom = document.createElement('div');
      dom.id = "bb_" + addr;
      dom.className = "basicblock eny0-selectable ec_gui_background ec_gui_border";
      dom.innerHTML = "<div class='instruction eny0-selectable'><span class='insaddr datainstruction ec_offset addr addr_0x" + addr.toString(16) + "' >0x" + addr.toString(16) + "</span></div>";
      this.vertices[addr].rendered = dom;
    }
  }
  if (vlen !== undefined) {
    this.vertices[addr].len = vlen;
    this.vertices[addr].rendered = dom;
  }
};

BBGraph.prototype.addEdge = function(v1, v2, color) {
  this.addVertex(v1);
  this.addVertex(v2);
  this.edges.push({'from': v1, 'to': v2, 'color': color});
  this.vertices[v1].children.push(v2);
  this.vertices[v2].parents.push(v1);
};

BBGraph.prototype.makeElement = function(addr, width, height, html) {
  this.elements.push(new joint.shapes.html.Element({
      id: String(addr),
      size: { width: width, height: height },
      html: html
  }));
};

BBGraph.prototype.makeLink = function(v1, v2, color) {
  this.links.push(new joint.dia.Link({
      source: { id: String(v1) },
      target: { id: String(v2) },
      attrs: {
          '.marker-target': {
              d: 'M 4 0 L 0 2 L 4 4 z',
              fill: color,
              stroke: color
          },
          '.connection': {
              'stroke-width': 1,
              stroke: color
          }
      },
      smooth: true
  }));
};

BBGraph.prototype.render = function() {
  var name = Object.keys(this.vertices).toString();
  var outergbox = document.createElement('div');
  outergbox.id = 'outergbox';
  var bbcanvas = document.getElementById("canvas");
  var gbox = document.createElement('div');
  gbox.id = 'gbox';
  gbox.className = name;
  outergbox.appendChild(gbox);
  bbcanvas.appendChild(outergbox);
  for (var addr in this.vertices) {
    var r = this.vertices[addr].rendered;
    if (r !== undefined) {
      gbox.appendChild(r);
      this.makeElement(addr, r.offsetWidth, r.offsetHeight, r.outerHTML);
    }
  }
  for (var j = 0; j < this.edges.length; j++) {
    this.makeLink(this.edges[j].from, this.edges[j].to, this.edges[j].color);
  }
  $("#outergbox").remove();

  this.makeElement("minimap_area", 1, 1, "<div id='minimap_area'>");

  var items = this.elements.concat(this.links);
  var width = $("#center_panel").width();
  var graph = new joint.dia.Graph();
  var paper = new joint.dia.Paper({
      el: $('#canvas'),
      gridSize: 1,
      width: 10000,
      height: 10000,
      model: graph,
  });

  var minimap_width = 200;
  var minimap_heigh = 200;
  $('#minimap').html("");
  $('#minimap').html("");
  var minimap = new joint.dia.Paper({
      el: $('#minimap'),
      gridSize: 1,
      width: minimap_width,
      height: minimap_heigh,
      model: graph
  });

  graph.resetCells(items);

  // render graph
  joint.layout.DirectedGraph.layout(graph);

  r2ui.graph_panel.graph = graph;

  // reposition graph
  reposition_graph();

  // remove html mask in minimap since its not scaled
  $("#minimap .basicblock").remove();

  // make minimap rect transparent
  graph.getCell("minimap_area").attr({rect: { stroke: "transparent"}});

  var svg_width = $('#canvas svg')[0].getBBox().width;
  var svg_height = $('#canvas svg')[0].getBBox().height;
  // update paper size with these values
  paper.setDimensions(svg_width + 5000, svg_height + 5000);
  var ws = Math.ceil(svg_width/minimap_width);
  var hs = Math.ceil(svg_height/minimap_heigh);
  var scale = 1/Math.max(ws, hs);
  var delta = 0;
  if (hs > ws) delta = (minimap_width/2) - svg_width*scale/2;
  minimap.scale(scale);
  minimap.setOrigin(delta,0);
  // minimap.$el.css('pointer-events', 'none');

  $("#minimap").css("left", $("#main_panel").width() - minimap_width);
  $("#minimap").css("top",  $("#center_panel").position().top - 40);
  //$("#center_panel").bind('scroll', update_minimap);

  paper.on( "cell:pointerup", function( cellview, evt, x, y)  {
    var model = cellview.model;
    var bbox = model.attributes.position;
    var id = String(model.prop("id"));
    if (model !== undefined && id !== "minimap_area") {
      var color = null;
      var bb = r2ui.get_fcn_BB(r2ui.current_fcn_offset, id);
      if (bb !== undefined && bb !== null) {
        if (bb.x != String(bbox.x) || bb.y != String(bbox.y)) {
          bb.x = bbox.x;
          bb.y = bbox.y;
          r2ui.update_fcn_BB(r2ui.current_fcn_offset, id, bb);
        }
      } else  if (bb !== undefined && bb !== null) {
        r2ui.update_fcn_BB(r2ui.current_fcn_offset, id, {x:bbox.x, y:bbox.y});
      }
    }
  });

  if (r2ui.graph_panel.minimap) {
    update_minimap();
    $("#minimap_area").draggable({
      containment: "parent",
      stop: function( event, ui ) {
        var delta_x = ui.position.left/scale;
        var delta_y = ui.position.top/scale;
        if (delta_x < 0) delta_x = 0;
        if (delta_y < 0) delta_y = 0;
        if ($("#radareApp_mp").length) {
          //$("#main_panel").scrollTo({ top:delta_y, left:delta_x - delta/scale } );
          console.log(1);
        } else {
          //$('#center_panel').scrollTo({ top:delta_y, left:delta_x - delta/scale } );
          console.log('debug:');
          console.log(delta_y, delta_x, scale);
          console.log($('#center_panel'));
        }
      }
    });
  } else {
    $("#minimap").hide();
  }
  
};

// Functions

function toggle_minimap() {
  if (r2ui.graph_panel.minimap) {
    r2ui.graph_panel.minimap = false;
    r2ui.seek(r2ui.graph_panel.selected_offset, false);
    $('#minimap').hide();
  } else {
    r2ui.graph_panel.minimap = true;
    r2ui.seek(r2ui.graph_panel.selected_offset, false);
    $('#minimap').show();
  }
}

function update_minimap() {
  if (r2ui.graph_panel.minimap && $('#canvas svg').length) {
    var minimap_width = 200;
    var minimap_height = 200;
    var svg_width = $('#canvas svg')[0].getBBox().width;
    var svg_height = $('#canvas svg')[0].getBBox().height;
    var ws = Math.ceil(svg_width/minimap_width);
    var hs = Math.ceil(svg_height/minimap_height);
    var scale = 1/Math.max(ws, hs);
    var delta = 0;
    if (hs > ws) delta = (minimap_width/2) - svg_width*scale/2;
    var el = null;
    // panel layout
    el = $('#center_panel');
    if (el.scrollTop() < svg_height) {
      $("#minimap_area").width(el.width()*scale);
      $("#minimap_area").height(el.height()*scale);
      if (el.scrollTop()*scale <= minimap_height - el.height()*scale)
        $("#minimap_area").css("top", el.scrollTop()*scale);
      $("#minimap_area").css("left", delta + el.scrollLeft()*scale);
    }
    el = $('#center_panel');
    $("#minimap").css("left", el.scrollLeft() + $("#main_panel").width() - minimap_width);
    $("#minimap").css("top",  el.scrollTop());
    $("#minimap").css("border", "1px solid " + r2ui.colors['.ec_gui_background']);
    $("#minimap_area").css("background", r2ui.colors['.ec_gui_background']);
  }
}

function reposition_graph() {
  var bbs = r2ui.graph_panel.graph.getElements();
  var blocks = r2ui.get_fcn_BBs(r2ui.current_fcn_offset);
  var bb_offsets = Object.keys(blocks);
  for (var i in bbs) {
    found = false;
    for (var j in bb_offsets) {
      var offset = String(bb_offsets[j]);
      var bb = blocks[offset];
      if (bbs[i].prop("id") === offset) {
        found = true;
        if (bb.x !== "null" && bb.y !== "null") {
          bbs[i].translate(bb.x - bbs[i].prop("position").x, bb.y - bbs[i].prop("position").y);
        }
        var color = bb.color;
        if (color !== null && color !== undefined) bbs[i].attr('rect/fill', color);
      }
    }
  }
}

var flag = 0;

function render_graph(x) {
  var obj;
  try {
    obj = JSON.parse(x.replace(/\\l/g,'\\n'));
  } catch (e) {
    console.log("Cannot parse JSON data");
  }
  if (obj[0] === undefined) return false;
  if (obj[0].blocks === undefined) return false;
  var bbgraph = new BBGraph();
  r2ui.current_fcn_offset = obj[0].blocks[0].ops[0].offset;

  for (var bn = 0; bn < obj[0].blocks.length; bn++) {
    var bb = obj[0].blocks[bn];
    var addr = bb.offset;
    if (bb.trace !== undefined) {
      var bbinfo = r2ui.get_fcn_BB(r2ui.current_fcn_offset, addr);
      if (bbinfo !== undefined) {
        if (bbinfo.color !== "red")
          bbinfo.color = "#7592DF";
      } else {
        bbinfo = {x:null, y:null, color:"#7592DF"};
      }
      r2ui.update_fcn_BB(r2ui.current_fcn_offset, addr, bbinfo);
    }
    if (bb.length === 0) continue;

    var cnt = bb.ops.length;
    var idump = "";
    for (var i in bb.ops) {
      var ins = bb.ops[i];
      // ins.offset = "0x" + ins.offset.toString(16);
      if (ins.comment === undefined || ins.comment === null) ins.comment = "";
      else {
        ins.comment = atob(ins.comment);
      }
      idump += html_for_instruction(ins);
    }
    var dom = document.createElement('div');
    dom.id = "bb_" + addr;
    dom.className = "basicblock eny0-selectable ec_gui_background ec_gui_border";
    dom.innerHTML = idump;
    bbgraph.addVertex(addr, cnt, dom);
    if (bb.fail > 0) {
      bbgraph.addEdge(addr, bb.fail, "red");
      if (bb.jump > 0) {
        bbgraph.addEdge(addr, bb.jump, "green");
      }
    } else if (bb.jump > 0) {
      bbgraph.addEdge(addr, bb.jump, "blue");
    }
  }
  bbgraph.render();

  var element = $("#canvas svg g .element");
  element.on("mousedown", function(event){
      flag = 0;
  });
  element.on("mousemove", function(event){
      flag = 1;
  });
  element.on("mouseup", function(event){
    if(flag === 0){
      var id = event.target.parentNode.parentNode.parentNode.getAttribute("model-id");
      if (id !== "minimap_area") {
        var color = null;
        var bb = r2ui.get_fcn_BB(r2ui.current_fcn_offset, id);
        if (bb !== undefined && bb !== null) {
          if (bb.color === "red") bb.color = r2ui.colors['.ec_gui_alt_background'];
          else bb.color = "red";
        } else {
          bb = {x:"null", y:"null", color:"red"};
        }
        r2ui.update_fcn_BB(r2ui.current_fcn_offset, id, bb);
        reposition_graph();
      }
    }
  });
  $(".addr").css("-webkit-user-select", "text");
  return true;
}

function render_instructions(instructions) {
  var outergbox = document.createElement('div');
  outergbox.id = 'outergbox';
  var flatcanvas = document.getElementById("canvas");
  flatcanvas.innerHTML = "";
  var gbox = document.createElement('div');
  gbox.id = 'gbox';
  gbox.className = 'ec_gui_background';
  outergbox.appendChild(gbox);
  flatcanvas.appendChild(outergbox);

  var flatcanvas_rect = getOffsetRect(flatcanvas);
  var asm_lines = (r2.settings["asm.lines"]);
  var asm_offset = (r2.settings["asm.offset"]);

  var accumulated_heigth = flatcanvas_rect.top;
  var lines = [];
  var targets = {};
  var first_address = instructions[0].offset;
  var last_address = instructions[instructions.length - 1].offset;
  for (var i in instructions) {
    var ins = instructions[i];

    if ((ins.type == "jmp" || ins.type == "cjmp") && ins.jump !== undefined && ins.jump !== null) {
      var line = {};
      line.from = ins.offset;
      if (last_address < ins.jump) {
        line.to_end = false;
        line.to = last_address;
      } else if (first_address > ins.jump) {
        line.to_end = false;
        line.to = first_address;
      } else {
        line.to_end = true;
        line.to = ins.jump;
      }
      if (ins.type == "jmp") {
        line.color = r2ui.colors[".ec_flow"];
        line.dashed = false;
      } else if (ins.type == "cjmp") {
        line.color = r2ui.colors[".ec_gui_cflow"];
        line.dashed = true;
      }
      line.to_start = true;
      lines[lines.length] = line;
      if (targets[line.to] === undefined) {
        targets[line.to] = 0;
      }
    }

    // ins.offset = "0x" + ins.offset.toString(16);
    if (ins.comment === undefined || ins.comment === null) ins.comment = "";
    else {
      try {
        ins.comment = atob(ins.comment);
      } catch(e) {
        console.log(ins.comment);
      }
    }
    var dom = document.createElement('div');
    if (asm_lines) dom.className = "instructionbox eny0-selectable lines";
    else dom.className = "instructionbox";
    dom.style.top = accumulated_heigth + "px";
    dom.innerHTML = html_for_instruction(ins);

    gbox.appendChild(dom);
    var instruction_rect = getOffsetRect(dom);
    var instruction_heigth = instruction_rect.bottom - instruction_rect.top;
    accumulated_heigth += instruction_heigth;
  }


  if (asm_lines) {
    var canvas = document.createElement("canvas");
    canvas.width = 500;
    canvas.height = accumulated_heigth;
    canvas.id = "linecanvas";
    canvas.setAttribute("tabindex", "1");
    canvas.setAttribute("style", "outline: none;");
    gbox.appendChild(canvas);
    var ctx = canvas.getContext("2d");
    if (!ctx.setLineDash) {
      // For browsers that dont support dashed lines
      ctx.setLineDash = function () {};
    }
    var num_targets = countProperties(targets);
    var num_assigned_paths = 0;
    var lines_width = 100;
    for (var l in lines) {
      line = lines[l];
      var from = "0x" + line.from.toString(16);
      var to = "0x" + line.to.toString(16);

      if (targets[line.to] === 0) {
        // No path assigned for target, assigning a new one
        targets[line.to] = (num_targets - num_assigned_paths - 1)*(90/(num_targets+1));
        num_assigned_paths += 1;
      }
      var from_element = get_element_by_address(from);
      var to_element = get_element_by_address(to);

      if (from_element !== null && from_element !== undefined && to_element !== undefined && to_element !== null) {
        var x = targets[line.to];
        var from_rect = getOffsetRect(from_element);
        var y0 = (from_rect.top + from_rect.bottom) / 2;
        var to_rect = getOffsetRect(to_element);
        var y1 = (to_rect.top + to_rect.bottom) / 2;
        if (line.to == instructions[0].offset) {
          y1 = 0;
        }

        // main line
        ctx.beginPath();
        ctx.moveTo(x, y0);
        ctx.lineTo(x, y1);
        ctx.strokeStyle = line.color;
        if (line.dashed) ctx.setLineDash([2,3]);
        ctx.stroke();

        if (line.to_start) {
          // horizontal line at start
          ctx.beginPath();
          ctx.moveTo(x, y0);
          ctx.lineTo(lines_width - 5, y0);
          ctx.strokeStyle = line.color;
          if (line.dashed) ctx.setLineDash([2,3]);
          ctx.stroke();

          // circle
          ctx.beginPath();
          ctx.arc(lines_width - 5 - 2, y0, 2, 0, 2 * Math.PI, false);
          ctx.fillStyle = line.color;
          ctx.fill();
        }

        if (line.to_end) {
          // horizontal line at end
          ctx.beginPath();
          ctx.moveTo(x, y1);
          ctx.lineTo(lines_width - 5, y1);
          ctx.strokeStyle = line.color;
          if (line.dashed) ctx.setLineDash([2,3]);
          ctx.stroke();

          // arrow
          ctx.beginPath();
          ctx.moveTo(lines_width - 5, y1);
          ctx.lineTo(lines_width - 10, y1-5);
          ctx.lineTo(lines_width - 10, y1+5);
          ctx.lineWidth = 1;
          ctx.fillStyle = line.color;
          ctx.fill();
        }
      }
    }
  }
  if (!asm_offset) {
    var elements = document.getElementsByClassName("insaddr");
    for (var j in elements) {
      if (elements[j].style) elements[j].style.display="none";
    }
  }
  $(".addr").css("-moz-user-select", "text");
  $(".addr").css("-webkit-user-select", "text");
}

function getOffsetRect(elem) {
    var box = elem.getBoundingClientRect();
    var offset = $('#gbox').offset().top;
    var top  = box.top - offset;
    var bottom  = box.bottom - offset;
    return {top: Math.round(top), bottom: Math.round(bottom)};
}

function countProperties(obj) {
  var count = 0;
  for(var prop in obj) {
    if(obj.hasOwnProperty(prop)) {
      ++count;
    }
  }
  return count;
}

function toBoolean(str) {
  if (str === "true") return true;
  else if (str === "false") return false;
  else return undefined;
}

function html_for_instruction(ins) {
  var idump = '<div class="instruction eny0-selectable">';
  var offset = "0x" + ins.offset.toString(16);
  var address = offset;
  var asm_flags = (r2.settings["asm.flags"]);
  var asm_bytes = (r2.settings["asm.bytes"]);
  var asm_xrefs = (r2.settings["asm.xrefs"]);
  var asm_cmtright = (r2.settings["asm.cmtright"]);

  if (ins.fcn_addr > 0 && offset === "0x"+ins.fcn_addr.toString(16)) {

    // Get Instruction info
    var results;
    var cmd = "afij " + offset;
    r2.cmd(cmd, function(x){
      results = x.split("\n");
    });
    try {
        var info = JSON.parse(results[0]);
        if (info !== null && info !== undefined && info.length > 0)
        idump += '<div class="ec_fname">(fcn) ' + info[0].name + '</div>';
    } catch (err) {
        console.log("Error getting instruction information from afij command");
    }

    // Get function variables
    var vars_reg_based = [];
    cmd = "afvrj " + offset;
    r2.cmd(cmd, function(x){
      vars_reg_based = x.split("\n");
    });
    try {
        var vars = JSON.parse(vars_reg_based[0]);
        var fvars = [];
        for (var i in vars) {
        idump += '<div class="ec_flag">; ' + vars[i].kind + " " + vars[i].type  + " <span class='fvar id_" + address_canonicalize(offset) + "_" + vars[i].ref + " ec_prompt faddr faddr_" + address_canonicalize(offset) + "'>" + escapeHTML(vars[i].name) + "</span> @ " + vars[i].ref + '</div>';
        fvars[fvars.length] = {name: vars[i].name, id:  address_canonicalize(offset) + "_" + vars[i].ref};
        }
        r2.varMap[ins.fcn_addr] = fvars;
    } catch (err) {
        console.log("Error getting variable information from afvj command");
    }

    // Get function arguments
    var args_merged = [];
    var args_bp_based = [];
    var args_sp_based = [];
    cmd = "afvbj " + offset;
    r2.cmd(cmd, function(x){
      args_bp_based = x.split("\n");
    });
    cmd = "afvsj " + offset;
    r2.cmd(cmd, function(x){
      args_sp_based = x.split("\n");
    });
    args_merged = args_bp_based.concat(args_sp_based);
    try {
        var args = JSON.parse(args_merged[0]);
        var fargs = [];
        for (var i in args) {
        idump += '<div class="ec_flag">; ' + args[i].kind + " " + args[i].type  + " <span class='farg id_" + address_canonicalize(offset) + "_" + args[i].ref + " ec_prompt faddr faddr_" + address_canonicalize(offset) + "'>" + escapeHTML(args[i].name) + "</span> @ " + args[i].ref + '</div>';
        fargs[fargs.length] = {name: args[i].name, id:  address_canonicalize(offset) + "_" + args[i].ref};
        }
        r2.argMap[ins.fcn_addr] = fargs;
    } catch (err) {
        console.log("Error getting argument information from afaj command");
    }
  }
  if (asm_flags) {
    var flags;
    if (ins.flags !== undefined && ins.flags !== null) {
      flags = ins.flags.join(";");
    } else {
      flags = r2.get_flag_names(address_canonicalize(offset)).join(";");
    }
    if (flags !== "" && flags !== undefined && flags !== null) idump += '<div class="ec_flag flags_' + address_canonicalize(offset) + '">;-- ' + escapeHTML(flags) + ':</div> ';
  }
  if (ins.comment && !asm_cmtright) {
    idump += '<div class="comment ec_comment comment_' + address_canonicalize(offset) + '">; ' + escapeHTML(ins.comment) + '</div>';
  }
  if (asm_xrefs) {
    if (ins.xrefs !== undefined && ins.xrefs !== null && ins.xrefs.length > 0) {
      var xrefs = "";
      for (var i in ins.xrefs) {
        var xref = ins.xrefs[i];
        var name = '';
        var xrefoffset = "0x"+xref.addr.toString(16);
        if (r2.get_flag_names(address_canonicalize(xrefoffset)).length > 0) name = ' (' + r2.get_flag_names(address_canonicalize(xrefoffset)).join(";") + ')';
        idump += '<div class="ec_flag xrefs">; ' + xref.type.toUpperCase() + ' XREF from ' +
        '<span class="offset addr addr_' + address_canonicalize(xrefoffset) + '">' + xrefoffset + '</span> ' +  name + '</div> ';
      }

    }
  }

  idump += '<span class="insaddr datainstruction ec_offset addr addr_' + address_canonicalize(offset) + '">' + address + '</span> ';

  if (asm_bytes) {
    if (ins.bytes !== undefined && ins.bytes !== null && ins.bytes !== "") {
      var dorep = function(a) {
        if (a=="00") return '<span class="ec_b0x00">00</span>';
        if (a=="ff") return '<span class="ec_b0x00">ff</span>';
        if (a=="7f") return '<span class="ec_b0x00">7f</span>';
      };
      var bytes = ins.bytes.replace(new RegExp("(00)|(ff)|(7f)", "g"), dorep);
      idump += '<span class="bytes ec_other">' + bytes + '</span> ';
    }
  }

  var opcode = highlight_instruction(ins.opcode, true);
  if ((r2.varMap[ins.fcn_addr] !== null && r2.varMap[ins.fcn_addr] !== undefined && r2.varMap[ins.fcn_addr].length > 0) ||
      (r2.argMap[ins.fcn_addr] !== null && r2.argMap[ins.fcn_addr] !== undefined && r2.argMap[ins.fcn_addr].length > 0)) {
    for (var i in r2.varMap[ins.fcn_addr]) {
      var var_name = r2.varMap[ins.fcn_addr][i].name;
      var var_id = r2.varMap[ins.fcn_addr][i].id;
      opcode = opcode.replace(" " + var_name + " ", " <span class='fvar id_" + var_id + " ec_prompt faddr faddr_" + address_canonicalize(offset) + "'>" + escapeHTML(var_name) + "</span> ");
    }
    for (var i in r2.argMap[ins.fcn_addr]) {
      var arg_name = r2.argMap[ins.fcn_addr][i];
      var arg_id = r2.argMap[ins.fcn_addr][i].id;
      opcode = opcode.replace(" " + arg_name + " ", " <span id='fvar id_" + arg_id + " ec_prompt faddr faddr_" + address_canonicalize(offset) + "'>" + escapeHTML(arg_name) + "</span> ");
    }
  }

  if (ins.type !== undefined && ins.type !== null) {
    if (contains(math, ins.type)) ins.type = "math";
    if (contains(bin, ins.type)) ins.type = "bin";
    if (ins.type == "ill") ins.type = "invalid";
    if (ins.type == "null") ins.type = "invalid";
    if (ins.type == "undefined") ins.type = "invalid";
    if (ins.type == "ujmp") ins.type = "jmp";
    if (ins.type == "upush") ins.type = "push";
    if (ins.type == "upop") ins.type = "pop";
    if (ins.type == "ucall") ins.type = "call";
    if (ins.type == "lea") ins.type = "mov";
    // Add default color if we failed to identify op type
    if (!contains(known_types, ins.type)) ins.type = "other";
    idump += '<div class="instructiondesc ec_' + ins.type + '">' + opcode + '</div> ';
  } else {
    idump += '<div class="instructiondesc">' + opcode + '</div> ';
  }
  if (ins.ptr_info) {
    idump += '<span class="comment ec_comment comment_' + address_canonicalize(offset) + '">' + escapeHTML(ins.ptr_info) + '</span>';
  }

  if (ins.comment && asm_cmtright) {
    idump += '<span class="comment ec_comment comment_' + address_canonicalize(offset) + '"> ; ' + escapeHTML(ins.comment) + '</span>';
  }

  if (ins.type == "ret") {
    idump += "<div>&nbsp</div>";
  }

  idump += '</div>';
  return idump;
}

var math = ["add", "sub", "mul", "imul", "div", "idiv", "neg", "adc", "sbb", "inc", "dec", ".byte"];
var bin = ["xor", "and", "or", "not"];
var regs = ["EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI", "EIP", "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RIP"];
var known_types = ["fline","help","args","label","flow","prompt","input","btext","swi","comment","fname","flag","offset","other","b0x00","b0x7f","b0xff","math","bin","push","pop","jmp","cjmp","call","nop","ret","trap","invalid","cmp","reg","creg","mov","num"];

function escapeHTML(text) {
  var chr = { '"': '&quot;', '&': '&amp;', '<': '&lt;', '>': '&gt;' };
  var escaped = text? String(text).replace(/[\"&<>]/g, function (a) { return chr[a]; }): "";
  return escaped;
}

function highlight_instruction(line, instruction) {
  if (line === undefined) return "undefined";
  if (instruction === undefined) instruction = true;
  var ret = escapeHTML(line);

  // highlight registers and addresses
  var re = "(0x[0123456789abcdef]+)";
  // Map with register names as keys and HTML span representing the register as value
  var reps = {};
  if (instruction) {
    for (var i in regs) {
      // Loop all the registers
      var rep = '<span class="ec_reg">' + regs[i] +'</span>';
      reps[regs[i]] = rep;
      rep = '<span class="ec_reg">' + regs[i].toLowerCase() + '</span>';
      reps[regs[i].toLowerCase()] = rep;
    }
    for (i in reps) {
      // Complete regexp with registers
      re += "|(" + i + ")";
    }
  }
  function dorep(a) {
    // If address found
    if (a.substr(0, 2) == "0x") {
      // Look for imports or functions
      var syms = r2.get_flag_names(address_canonicalize(a));
      for (var i in syms) {
        if (syms[i].indexOf("sym.imp.")) {
          return "<span class='ec_offset addr addr_" + address_canonicalize(a) + "'>" + syms[i] + "</span>";
        }
        if (syms[i].indexOf("fcn.")) {
          return "<span class='ec_offset addr addr_" + address_canonicalize(a) + "'>" + syms[i] + "</span>";
        }
      }
      // Is it data address or code address
      var cl = get_data_type(a);
      if (cl === "") {
        // Just an hex value, not an address
        return "<span class='ec_num'>" + a + "</span>";
      } else if (cl === "datainstruction") {
        // An address representing data (memory) or code (instruction)
        return "<span class='ec_offset addr addr_" + address_canonicalize(a) + "'>" + a + "</span>";
      } else if (cl === "datamemory") {
        // return "<span class='ec_dataoffset addr addr_" + address_canonicalize(a) + "'>" + a + "</span>";
        return "<span class='ec_dataoffset'>" + a + "</span>";
      }
    } else {
      // Not an hex value, so a register
      return reps[a];
    }
  }
  return ret.replace(new RegExp(re, "g"), dorep);
}

function hex2(a) {
  if (a === undefined) return "__";
  var ret = a.toString(16);
  if (ret.length == 1) return "0" + ret;
  return ret;
}

function hex(a) {
  if (a === undefined) {
    return "";
  } else {
    if (a < 0) a += 0x100000000;
    return "0x"+a.toString(16);
  }
}

function get_data_type(v, more) {
  var a = r2.get_address_type(v);
  if (a === "") return "";
  else {
    if (more !== undefined) {
      return "data" + a + " addr addr_" + v;
    } else {
      return "data" + a;
    }
  }
}

function fnum(a) {
  return parseInt(a, 10);
}

function get_address_from_class(t, type) {
  if (type === undefined) type = "addr";
  var prefix = type+"_";
if (!t) return undefined;
  var l = t.className.split(" ").filter(function(x) { return x.substr(0,prefix.length) == type+"_"; });
  if (l.length != 1) return undefined;
  return l[0].split("_")[1].split(" ")[0];
}

function rehighlight_iaddress(address, prefix) {
  if (prefix === undefined) prefix = "addr";
  $('.autohighlighti').removeClass('autohighlighti');
  $('.' + prefix + '_' + address).addClass('autohighlighti');
  if (prefix === "addr") r2.cmd ("ss " + address, function () {});
}

function rehighlight_id(eid) {
  $('.autohighlighti').removeClass('autohighlighti');
  $('#' + eid).addClass('autohighlighti');
}

function get_element_by_address(address) {
  var elements = $(".insaddr.addr_" + address);
  if (elements.length === 1) return elements[0];
  else return null;
}

Element.prototype.documentOffsetTop = function () {
    return this.offsetTop + ( this.offsetParent ? this.offsetParent.documentOffsetTop() : 0 );
};

// function scroll_to_address(address) {
//   var elements = $(".insaddr.addr_" + address);
//   var top = elements[0].documentOffsetTop() - window.innerHeight / 2;
//   top = Math.max(0,top);
//   $("#main_panel").scrollTo({'top':top, 'left':0});
// }

// function has_scrollbar(divnode) {
//   if(divnode.scrollHeight > divnode.clientHeight) return true;
//   return false;
// }

// function on_scroll(event) {
//   if (!r2ui.graph_panel.scrolling) {
//     var panel_disas = $("#main_panel").tabs("option", "active") === 0 ? true : false;
//     event.preventDefault();
//   }
// }

// function scroll_to_element(element) {
//   if (element === undefined || element === null) return;
//   var top = Math.max(0,element.documentOffsetTop() - ( window.innerHeight / 2 ));
//   $('#center_panel').scrollTo(top, {axis: 'y'});
//   r2ui.graph_panel.scroll_offset = top;
// }
//
// function store_scroll_offset() {
//   r2ui.graph_panel.scroll_offset = $('#center_panel').scrollTop();
// }
//
// function scroll_to_last_offset() {
//   if (r2ui.graph_panel.scroll_offset !== null) $('#center_panel').scrollTo(r2ui.graph_panel.scroll_offset, {axis: 'y'});
// }

function rename(offset, old_value, new_value, space) {
  if (space === undefined) space = "functions";
  if (space == "functions") {
    // If current offset is the beginning of a function, rename it with afr
    r2.cmdj("pdfj @ " + offset, function(x) {
      if (x !== null && x !== undefined) {
        if ("0x" + x.addr.toString(16) === offset) {
          r2.cmd("afn " + new_value + " " + offset, function() {
            r2.update_flags();
            return;
          });
        }
      }
    });
  }
  // Otherwise just add a flag
  var cmd = "";
  if (new_value !== "" && old_value !== "") {
    cmd = "fs " + space + ";fr " + old_value + " " + new_value;
    r2.cmd(cmd, function() {});
  } else if (new_value === "" && old_value !== "") {
    cmd = "fs " + space + ";f-@" + offset;
    r2.cmd(cmd, function() {});
  } else if (new_value !== "" && old_value === "") {
    cmd = "fs " + space + ";f " + new_value + " @ " + offset;
    r2.cmd(cmd, function() {});
  }
  r2.update_flags();
}

function address_canonicalize(s) {
  s = s.substr(2);
  while (s.substr(0,1) == '0') s = s.substr(1);
  s = "0x" + s;
  s = s.toLowerCase();
  return s;
}

function contains(a, obj) {
    for (var i = 0; i < a.length; i++) {
        if (a[i] === obj) {
            return true;
        }
    }
    return false;
}

function handleInputTextChange() {
  r2ui.graph_panel.handleInputTextChange();
}

function show_contextMenu(x,y) {
  r2ui.graph_panel.showContextMenu(x,y);
}

function get_offset_flag(offset) {
  var old_value = "";
  r2.cmdj("fs offsets;fj", function(x) {
    for (var i in x) {
      if ("0x" + x[i].offset.toString(16) == offset) {
        old_value = x[i].name;
        break;
      }
    }
  });
  return old_value;
}

function get_symbol_flag(symbol) {
  var full_name = symbol;
  var found = false;
  r2.cmdj("fs symbols;fj", function(x) {
    for (var i in x) {
      if (x[i].name == symbol) {
        found = true;
        break;
      }
    }
    if (!found) {
      for (var i in x) {
        if (x[i].name == "sym." + symbol) {
          full_name = "sym." + symbol;
          break;
        }
      }
    }
  });
  return full_name;
}

function get_reloc_flag(reloc) {
  var full_name = reloc;
  var found = false;
  r2.cmdj("fs relocs;fj", function(x) {
    for (var i in x) {
      if (x[i].name == reloc) {
        found = true;
        break;
      }
    }
    if (!found) {
      for (var i in x) {
        if (x[i].name == "reloc." + reloc) {
          full_name = "reloc." + reloc;
          break;
        }
      }
    }
  });
  return full_name;
}



