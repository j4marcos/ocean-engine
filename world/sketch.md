

class entite 

class point 

originPoint = new point(0,0,0)

class direction //vetor ou degres

// alguma coisa no espaço
class element {
    point = originPoint
    radios = 1
    rotation = 
    velocit = 0
    direction forceDirection = new direction() // direção resultando das forças aplicadas ao objeto

    element (point = new point(), radios = 0.0) {

    ...

    render()
    }

    setRotation ()

    render ()

}

// shape é figura 2d
class shape extends element {
setColor (rgb: RGB)
}

// shape é elemento 3d
class Form extends element {
    setColor (rgb: RGB)
}

class circulo extends shape {
circulo(ponto, diametro)
}

class esfere extends form {

    esfere (ponto, diametro)

}




class camera extends element {

    camera (point = originPoint)

    follow(element) { // posição da camera segue o elemento mantendo a mesma distancia e direção em relação ao elemento de antes de follow ser ativado

    }

    lookAt(direction) // camera olha pra direção 

    lookAt(element) // cemra olha pra o objeto
}

class entite () { // alma de um elemento pra seres vivos

}

class body extends form {

}

player extends entite () {
    body new body()


    // aceita comandos do teclado
}

void main() {

    star = new esfere()
    esfere.setRadios = 100.0
    esfere.setPosition = new point()

    planet = new esfere()
    planet.setPosition (new Point(10, 0, 0) )
    planet.setRadios ( 5.0 )

    ringTotalRadios = planet.radios * 1.5 
    ringInternalRadios = planet.radios * 1.2
    ringCenter = planet.center 
    externalCircle  = new circle().setPosition(ringCenter).setRadios(ringRadios)
    internalCircle = new circle().setPosition(ringCenter).setRadios(ringInternalRadios)
    ring = circuloExterno - circulo interno // subtração geometrica 

    player = new Player()
    player.setPosition(new point())

    camera = new Camera() 
    camera.follow(player)
    camera.lookAt(player)

    camera.playWindow()

}