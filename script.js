var Sidebar = function(editor) {
    var signals = editor.signals;

    var container = new UI.Panel();

    var buttons = new UI.Panel();
    container.add(buttons);

    var translateButton = new UI.Button('translate').onClick(function() {
        signals.transformModeChanged.dispatch( 'translate' );
    });

    var rotateButton = new UI.Button('rotate').onClick(function() {
        signals.transformModeChanged.dispatch( 'rotate' );
    });

    var addCameraButton = new UI.Button('add camera').onClick(function() {
        dataEditor.addCamera();
    });

    var saveButton = new UI.Button('save').onClick(function() {
        dataEditor.save();
    });

    buttons.add(translateButton);
    buttons.add(rotateButton);
    buttons.add(addCameraButton);
    buttons.add(saveButton);



    var cameras = new UI.Panel();
    container.add(cameras);

    cameras.add(new UI.Text('Cameras'));
    cameras.add( new UI.Break() );
    cameras.add( new UI.Break() );

    var outliner = new UI.FancySelect().setId( 'outliner' );
    outliner.onChange( function () {
        editor.selectByUuid(outliner.getValue())
    } );
    cameras.add(outliner);

    var objects = new UI.Panel();
    container.add(objects);

    var objectPositionRow = new UI.Panel();
    var objectPositionX = new UI.Number().setWidth( '50px' ).onChange( update );
    var objectPositionY = new UI.Number().setWidth( '50px' ).onChange( update );
    var objectPositionZ = new UI.Number().setWidth( '50px' ).onChange( update );

    objectPositionRow.add( new UI.Text( 'Position' ).setWidth( '90px' ) );
    objectPositionRow.add( objectPositionX, objectPositionY, objectPositionZ );

    var objectRotationRow = new UI.Panel();
    var objectRotationX = new UI.Number().setWidth( '50px' ).onChange( update );
    var objectRotationY = new UI.Number().setWidth( '50px' ).onChange( update );
    var objectRotationZ = new UI.Number().setWidth( '50px' ).onChange( update );

    objectRotationRow.add( new UI.Text( 'Rotation' ).setWidth( '90px' ) );
    objectRotationRow.add( objectRotationX, objectRotationY, objectRotationZ );

    var scaleRow = new UI.Panel();
    var scale = new UI.Number().setWidth('50px').onChange(function() {
        dataEditor.scale = scale.getValue();
    });
    scale.setValue(dataEditor.scale);

    scaleRow.add(new UI.Text('Scale').setWidth('90px'));
    scaleRow.add(scale);

    objects.add( objectPositionRow );
    objects.add( objectRotationRow );
    objects.add(scaleRow);

    /* mcon panel */
    var constPanel = new UI.Panel();
    var mconValue = new UI.Number(1).setWidth( '50px' );
    constPanel.add( new UI.Text( 'mcon:' ).setWidth( '50px' ) );
    constPanel.add( mconValue );
    var nconValue = new UI.Number(0).setWidth( '50px' );
    constPanel.add( new UI.Text( 'ncon:' ).setWidth( '50px' ) );
    constPanel.add( nconValue );
    constPanel.add( new UI.Break() );
    var isInit = new UI.Checkbox(true);
    constPanel.add( new UI.Text( 'init?:' ).setWidth( '50px' ) );
    constPanel.add( isInit );
    container.add( constPanel );


    var output = new UI.Panel();
    var headerRow = new UI.Panel();
    var row = new UI.Panel();
    container.add(output);

    var run = new UI.Button('Run').onClick(function() {
        results.scale = dataEditor.scale;
        var r = qt.run(dataEditor.getParams(), nconValue.getValue(),
                       mconValue.getValue(), isInit.getValue() );
        console.log(r);
        results.loadFromArray(r);
    });
    run.setClass('right');

    dataEditor.output = new UI.TextArea();
    dataEditor.output.setWidth('98%');
    dataEditor.output.setHeight('100px');

    headerRow.add(new UI.Text("Output"));
    headerRow.add(run);
    output.add(headerRow);
    output.add( new UI.Break() );
    output.add(row);
    row.add(dataEditor.output);

    var input = new UI.Panel();
    headerRow = new UI.Panel();
    var dataRow = new UI.Panel();
    input.add(headerRow);
    input.add(dataRow);
    container.add(input);

    var applyButton = new UI.Button('Apply').onClick(function() {
        dataEditor.load(text.getValue());
    });
    applyButton.setClass('right');

    headerRow.add(new UI.Text('Input'));
    headerRow.add(applyButton);

    var text = new UI.TextArea();
    text.setWidth('98%');
    text.setHeight('100px');

    dataRow.add(text);

    function update() {
        var object = editor.selected;
        if(!object) {
            return;
        }
        object.position.x = objectPositionX.getValue();
        object.position.y = objectPositionY.getValue();
        object.position.z = objectPositionZ.getValue();

        object.rotation.x = objectRotationX.getValue();
        object.rotation.y = objectRotationY.getValue();
        object.rotation.z = objectRotationZ.getValue();
        signals.objectChanged.dispatch( object );
    }

    signals.sceneGraphChanged.add( function () {
        var opt = {};
        if(!dataEditor) {
            return;
        }
        dataEditor.cameras.forEach(function(camera) {
            opt[camera.uuid] = camera.name;
        });
        outliner.setOptions(opt);
        if ( editor.selected !== null ) {
            outliner.setValue(editor.selected.uuid);
        }
    });

    signals.objectSelected.add( function ( object ) {
        if(dataEditor.cameras.indexOf(object) === -1) {
            return;
        }
        outliner.setValue( object !== null ? object.uuid : null );
        objectPositionX.setValue(object.position.x);
        objectPositionY.setValue(object.position.y);
        objectPositionZ.setValue(object.position.z);
        objectRotationX.setValue(object.rotation.x);
        objectRotationY.setValue(object.rotation.y);
        objectRotationZ.setValue(object.rotation.z);
    } );

    signals.objectChanged.add(function(object) {
        if(dataEditor.cameras.indexOf(object) === -1 && results.cameras.indexOf(object) === -1) {
            return;
        }
        object.userData.lines.forEach(function(line) {
            line.geometry.verticesNeedUpdate = true;
        });
        objectPositionX.setValue(object.position.x);
        objectPositionY.setValue(object.position.y);
        objectPositionZ.setValue(object.position.z);
        objectRotationX.setValue(object.rotation.x);
        objectRotationY.setValue(object.rotation.y);
        objectRotationZ.setValue(object.rotation.z);
    });

    return container;
}

var DataEditor = function(color) {
    this.color = color
    this.cameras = [];
    this.scale = 400;
    this.output = null;
}

DataEditor.prototype.addCamera = function() {
	 var geometry = new THREE.Geometry();
	 geometry.vertices.push(new THREE.Vector3(2, 2,3));
	 geometry.vertices.push(new THREE.Vector3(2, -2,3));
	 geometry.vertices.push(new THREE.Vector3(-2, -2,3));
	 geometry.vertices.push(new THREE.Vector3(-2, 2,3));
	 geometry.vertices.push(new THREE.Vector3(0, 0,-3));
	 
	 geometry.faces.push(new THREE.Face3(4, 0, 1));
	 geometry.faces.push(new THREE.Face3(4, 1, 2));
	 geometry.faces.push(new THREE.Face3(4, 2, 3));
	 geometry.faces.push(new THREE.Face3(4, 3, 0));
	 
	 geometry.faces.push(new THREE.Face3(0, 3, 1));
	 geometry.faces.push(new THREE.Face3(2, 1, 3));
	 //geometry.faces.push(new THREE.Face4(0, 1, 2, 3));
	 //geometry.faces.push(new THREE.Face4(0, 3, 2, 1));
	 geometry.computeFaceNormals();
	 	 
	 geometry.computeBoundingSphere();
    var camera = new THREE.Mesh(
        //new THREE.CylinderGeometry(10, 0, 12, 4, 1, false),
        geometry,
        new THREE.MeshPhongMaterial({emissive: this.color})
    );
    camera.name = "Camera "+(this.cameras.length+1);
    camera.userData.lines = [];
    if(this.cameras.length !== 0) {
        var lastCamera = this.cameras[this.cameras.length-1];
        camera.position.x = lastCamera.position.x;
        camera.position.y = lastCamera.position.y;
        camera.position.z = lastCamera.position.z;
        camera.rotation.x = lastCamera.rotation.x;
        camera.rotation.y = lastCamera.rotation.y;
        camera.rotation.z = lastCamera.rotation.z;
        var geom = new THREE.Geometry();
        geom.dynamic = true;
        geom.vertices.push(lastCamera.position);
        geom.vertices.push(camera.position);
        var line = new THREE.Line(geom, new THREE.LineBasicMaterial({color: 0, linewidth: 5}));
        editor.addObject(line);
        lastCamera.userData.lines.push(line);
        camera.userData.lines.push(line);
    }
    this.cameras.push(camera);
    editor.addObject(camera);
    editor.select(camera);
    return camera;
}

DataEditor.prototype.save = function() {
    var prevCamera = {
        position: {x: 0, y: 0, z: 0},
        rotation: {x: 0, y: 0, z: 0}
    };
    var result = '';

    for(var i=0; i<dataEditor.cameras.length; i++) {
        var curCamera = dataEditor.cameras[i];
        result += (curCamera.rotation.x - prevCamera.rotation.x).toFixed(6) + ' ';
        result += (curCamera.rotation.y - prevCamera.rotation.y).toFixed(6) + ' ';
        result += (curCamera.rotation.z - prevCamera.rotation.z).toFixed(6) + ' ';
        result += ((curCamera.position.x - prevCamera.position.x)/this.scale).toFixed(6) + ' ';
        result += ((curCamera.position.y - prevCamera.position.y)/this.scale).toFixed(6) + ' ';
        result += ((curCamera.position.z - prevCamera.position.z)/this.scale).toFixed(6) + '\n';
        prevCamera = curCamera;
    }

    this.output.setValue(result);
}

DataEditor.prototype.getParams = function() {
    var params = [];
    var prevCamera = {
        position: {x: 0, y: 0, z: 0},
        rotation: {x: 0, y: 0, z: 0}
    };
    for(var i=0; i<dataEditor.cameras.length; i++) {
        var curCamera = dataEditor.cameras[i];
        params.push(curCamera.rotation.x - prevCamera.rotation.x);
        params.push(curCamera.rotation.y - prevCamera.rotation.y);
        params.push(curCamera.rotation.z - prevCamera.rotation.z);
        params.push((curCamera.position.x - prevCamera.position.x)/this.scale);
        params.push((curCamera.position.y - prevCamera.position.y)/this.scale);
        params.push((curCamera.position.z - prevCamera.position.z)/this.scale);
        prevCamera = curCamera;
    }
    return params;
}
DataEditor.prototype.load = function(input) {
    console.log("just load");
    this.cameras.forEach(function(camera) {
        camera.userData.lines.forEach(function(line) {
            editor.removeObject(line);
        });
        editor.removeObject(camera);
    });
    this.cameras = [];
    var prevCamera = {
        position: {x: 0, y: 0, z: 0},
        rotation: {x: 0, y: 0, z: 0}
    };
    var values = input.match(/(-)*\d{1,}(\.\d{1,})*/g);
    for(var i=0; i<values.length; i+=6) {
        var camera = this.addCamera();
        camera.rotation.x = prevCamera.rotation.x+parseFloat(values[i+0]);
        camera.rotation.y = prevCamera.rotation.y+parseFloat(values[i+1]);
        camera.rotation.z = prevCamera.rotation.z+parseFloat(values[i+2]);
        camera.position.x = prevCamera.position.x+parseFloat(values[i+3])*this.scale;
        camera.position.y = prevCamera.position.y+parseFloat(values[i+4])*this.scale;
        camera.position.z = prevCamera.position.z+parseFloat(values[i+5])*this.scale;
        camera.userData.lines.forEach(function(line) {
            line.geometry.verticesNeedUpdate = true;
        });
        prevCamera = camera;
    }
    editor.select(this.cameras[this.cameras.length-1]);
};

DataEditor.prototype.test = function(a) {
    console.log(a);
}

DataEditor.prototype.loadFromArray = function(values) {
    console.log('load from array');
    this.cameras.forEach(function(camera) {
        camera.userData.lines.forEach(function(line) {
            editor.removeObject(line);
        });
        editor.removeObject(camera);
    });
    this.cameras = [];
    var prevCamera = {
        position: {x: 0, y: 0, z: 0},
        rotation: {x: 0, y: 0, z: 0}
    };
    console.log("values length: " + values.length);
    for(var i=0; i<values.length; i+=6) {
        console.log(values[i+0]);
        console.log(values[i+1]);
        console.log(values[i+2]);
        console.log(values[i+3]);
        console.log(values[i+4]);
        console.log(values[i+5]);
        var camera = this.addCamera();
        camera.rotation.x = prevCamera.rotation.x+values[i+0];
        camera.rotation.y = prevCamera.rotation.y+values[i+1];
        camera.rotation.z = prevCamera.rotation.z+values[i+2];
        camera.position.x = prevCamera.position.x+values[i+3]*this.scale;
        camera.position.y = prevCamera.position.y+values[i+4]*this.scale;
        camera.position.z = prevCamera.position.z+values[i+5]*this.scale;
        camera.userData.lines.forEach(function(line) {
            line.geometry.verticesNeedUpdate = true;
        });
        prevCamera = camera;
        editor.selectByUuid(camera.uuid);
    }
}
