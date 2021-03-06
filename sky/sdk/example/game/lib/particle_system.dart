part of sprites;

class _Particle {
  Vector2 pos;
  Vector2 startPos;

  double colorPos;
  double deltaColorPos;

  double size;
  double deltaSize;

  double rotation;
  double deltaRotation;

  double timeToLive;

  Vector2 dir;
  double radialAccel;
  double tangentialAccel;

  ColorSequence colorSequence;
}


class ParticleSystem extends Node {

  Texture texture;

  double life;
  double lifeVar;

  Point posVar;

  double startSize;
  double startSizeVar;

  double endSize;
  double endSizeVar;

  double startRotation;
  double startRotationVar;

  double endRotation;
  double endRotationVar;

  bool rotateToMovement;

  double direction;
  double directionVar;

  double speed;
  double speedVar;

  double radialAcceleration;
  double radialAccelerationVar;

  double tangentialAcceleration;
  double tangentialAccelerationVar;

  Vector2 gravity;

  int maxParticles;
  int numParticlesToEmit;
  double emissionRate;
  bool autoRemoveOnFinish;

  ColorSequence colorSequence;
  int alphaVar;
  int redVar;
  int greenVar;
  int blueVar;
  TransferMode colorTransferMode;
  TransferMode transferMode;

  List<_Particle> _particles;

  double _emitCounter;
  // Not yet used:
  // double _elapsedTime;
  int _numEmittedParticles = 0;

  ParticleSystem(this.texture,
                 {this.life: 1.5,
                  this.lifeVar: 1.0,
                  this.posVar: Point.origin,
                  this.startSize: 2.5,
                  this.startSizeVar: 0.5,
                  this.endSize: 0.0,
                  this.endSizeVar: 0.0,
                  this.startRotation: 0.0,
                  this.startRotationVar: 0.0,
                  this.endRotation: 0.0,
                  this.endRotationVar: 0.0,
                  this.rotateToMovement : false,
                  this.direction: 0.0,
                  this.directionVar: 360.0,
                  this.speed: 100.0,
                  this.speedVar: 50.0,
                  this.radialAcceleration: 0.0,
                  this.radialAccelerationVar: 0.0,
                  this.tangentialAcceleration: 0.0,
                  this.tangentialAccelerationVar: 0.0,
                  this.gravity,
                  this.maxParticles: 100,
                  this.emissionRate: 50.0,
                  this.colorSequence,
                  this.alphaVar: 0,
                  this.redVar: 0,
                  this.greenVar: 0,
                  this.blueVar: 0,
                  this.colorTransferMode: TransferMode.multiply,
                  this.transferMode: TransferMode.plus,
                  this.numParticlesToEmit: 0,
                  this.autoRemoveOnFinish: true}) {
    _particles = new List<_Particle>();
    _emitCounter = 0.0;
    // _elapsedTime = 0.0;
    if (gravity == null) gravity = new Vector2.zero();
    if (colorSequence == null) colorSequence = new ColorSequence.fromStartAndEndColor(new Color(0xffffffff), new Color(0x00ffffff));
  }

  void update(double dt) {
    // TODO: Fix this (it's a temp fix for low framerates)
    if (dt > 0.1) dt = 0.1;

    // Create new particles
    double rate = 1.0 / emissionRate;

    if (_particles.length < maxParticles) {
      _emitCounter += dt;
    }

    while(_particles.length < maxParticles
       && _emitCounter > rate
       && (numParticlesToEmit == 0 || _numEmittedParticles < numParticlesToEmit)) {
      // Add a new particle
      _addParticle();
      _emitCounter -= rate;
    }

    // _elapsedTime += dt;

    // Iterate over all particles
    for (int i = _particles.length -1; i >= 0; i--) {
      _Particle particle = _particles[i];

      // Manage life time
      particle.timeToLive -= dt;
      if (particle.timeToLive <= 0) {
        _particles.removeAt(i);
        continue;
      }

      // Update the particle

      // Radial acceleration
      Vector2 radial;
      if (particle.pos[0] != 0 || particle.pos[1] != 0) {
        radial = new Vector2.copy(particle.pos).normalize();
      } else {
        radial = new Vector2.zero();
      }
      Vector2 tangential = new Vector2.copy(radial);
      radial.scale(particle.radialAccel);

      // Tangential acceleration
      double newY = tangential.x;
      tangential.x = -tangential.y;
      tangential.y = newY;
      tangential.scale(particle.tangentialAccel);

      // (gravity + radial + tangential) * dt
      Vector2 accel = (gravity + radial + tangential).scale(dt);
      particle.dir += accel;
      particle.pos += new Vector2.copy(particle.dir).scale(dt);

      // Size
      particle.size = math.max(particle.size + particle.deltaSize * dt, 0.0);

      // Angle
      particle.rotation += particle.deltaRotation * dt;

      // Color
      particle.colorPos = math.min(particle.colorPos + particle.deltaColorPos * dt, 1.0);
    }

    if (autoRemoveOnFinish && _particles.length == 0 && _numEmittedParticles > 0) {
      if (parent != null) removeFromParent();
    }
  }

  void _addParticle() {

    _Particle particle = new _Particle();

    // Time to live
    particle.timeToLive = math.max(life + lifeVar * randomSignedDouble(), 0.0);

    // Position
    Point srcPos = Point.origin;
    particle.pos = new Vector2(srcPos.x + posVar.x * randomSignedDouble(),
                               srcPos.y + posVar.y * randomSignedDouble());

    // Size
    particle.size = math.max(startSize + startSizeVar * randomSignedDouble(), 0.0);
    double endSizeFinal = math.max(endSize + endSizeVar * randomSignedDouble(), 0.0);
    particle.deltaSize = (endSizeFinal - particle.size) / particle.timeToLive;

    // Rotation
    particle.rotation = startRotation + startRotationVar * randomSignedDouble();
    double endRotationFinal = endRotation + endRotationVar * randomSignedDouble();
    particle.deltaRotation = (endRotationFinal - particle.rotation) / particle.timeToLive;

    // Direction
    double dirRadians = convertDegrees2Radians(direction + directionVar * randomSignedDouble());
    Vector2 dirVector = new Vector2(math.cos(dirRadians), math.sin(dirRadians));
    double speedFinal = speed + speedVar * randomSignedDouble();
    particle.dir = dirVector.scale(speedFinal);

    // Radial acceleration
    particle.radialAccel = radialAcceleration + radialAccelerationVar * randomSignedDouble();

    // Tangential acceleration
    particle.tangentialAccel = tangentialAcceleration + tangentialAccelerationVar * randomSignedDouble();

    // Color
    particle.colorPos = 0.0;
    particle.deltaColorPos = 1.0 / particle.timeToLive;

    if (alphaVar != 0 || redVar != 0 || greenVar != 0 || blueVar != 0) {
      particle.colorSequence = new ColorSequence.copyWithVariance(colorSequence, alphaVar, redVar, greenVar, blueVar);
    }

    _particles.add(particle);
    _numEmittedParticles++;
  }

  void paint(PaintingCanvas canvas) {

    List<RSTransform> transforms = [];
    List<Rect> rects = [];
    List<Color> colors = [];

    for (_Particle particle in _particles) {
      // Transform
      double scos;
      double ssin;
      if (rotateToMovement) {
        double extraRotation = GameMath.atan2(particle.dir[1], particle.dir[0]);
        scos = math.cos(convertDegrees2Radians(particle.rotation) + extraRotation) * particle.size;
        ssin = math.sin(convertDegrees2Radians(particle.rotation) + extraRotation) * particle.size;
      } else {
        scos = math.cos(convertDegrees2Radians(particle.rotation)) * particle.size;
        ssin = math.sin(convertDegrees2Radians(particle.rotation)) * particle.size;
      }
      RSTransform transform = new RSTransform(scos, ssin, particle.pos[0], particle.pos[1]);
      transforms.add(transform);

      // Rect
      Rect rect = texture.frame;
      rects.add(rect);

      // Color
      Color particleColor;
      if (particle.colorSequence != null) {
        particleColor = particle.colorSequence.colorAtPosition(particle.colorPos);
      } else {
        particleColor = colorSequence.colorAtPosition(particle.colorPos);
      }
      colors.add(particleColor);
    }

    Paint paint = new Paint()..setTransferMode(transferMode)
        ..setFilterQuality(FilterQuality.low) // All Skia examples do this.
        ..isAntiAlias = false; // Antialiasing breaks SkCanvas.drawAtlas?
    canvas.drawAtlas(texture.image, transforms, rects, colors,
      TransferMode.modulate, null, paint);
  }
}
