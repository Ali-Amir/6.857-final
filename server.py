import pika
from collections import defaultdict
import time

R = [defaultdict(float), defaultdict(float)]
LAPTOP_DISTANCE = 5.0
PIXEL_DISTANCE = 200
WIDTH = 1024
HEIGHT = 800

Xmap = defaultdict(float)
Ymap = defaultdict(float)

alpha = 0.9

def rolling_update(dic, key, new_val, alpha):
  if not key in dic:
    dic[key] = new_val
  else:
    dic[key] = dic[key] * (1.0 - alpha) + new_val * alpha

def update(rid, mac, dis):
  rolling_update(R[rid], mac, dis, alpha)

######################
###### GEOMETRY ######
######################
def intersect(r0, r1):
  add = max(0.0, (LAPTOP_DISTANCE - r0 - r1)/2.0)
  r0 += add
  r1 += add

  dx = (max(0.0, r0**2.0 - ((r1**2.0 - r0**2.0 - LAPTOP_DISTANCE**2.0)**2.0) / ((2*LAPTOP_DISTANCE)**2.0)))**0.5

  dy = LAPTOP_DISTANCE/2.0 - ((r1**2.0 - dx**2.0)**0.5)
  return dx, dy

######################
######## UI ##########
######################
from Tkinter import *
root = Tk()
def drawcircle(canv,x,y,rad):
    canv.create_oval(x-rad,y-rad,x+rad,y+rad,width=0,fill='blue')

canvas = Canvas(width=WIDTH, height=HEIGHT, bg='white')
canvas.pack(expand=YES, fill=BOTH)
def ui_update():
  y0 = HEIGHT / 2 - PIXEL_DISTANCE / 2
  x0 = 40
  y1 = HEIGHT / 2 + PIXEL_DISTANCE / 2
  x1 = 40

  canvas.delete("all")
  circ1=drawcircle(canvas,x0, y0, 20)
  circ2=drawcircle(canvas,x1, y1, 20)
  for mac in R[0]:
    if not mac in R[1]:
      continue
    x, y = intersect(R[0][mac], R[1][mac])
    rolling_update(Xmap, mac, x, alpha)
    rolling_update(Ymap, mac, y, alpha)
    x, y = (Xmap[mac], Ymap[mac])
    x_cen = int(x * PIXEL_DISTANCE / LAPTOP_DISTANCE + 40)
    y_cen = int(y * PIXEL_DISTANCE / LAPTOP_DISTANCE + HEIGHT / 2)
    drawcircle(canvas, x_cen, y_cen, 30)

  text = canvas.create_text(50,10, text="tk test")
  root.update()

######################
######## RABBIT_MQ ##########
######################
credentials = pika.PlainCredentials('a', 'a')
connection = pika.BlockingConnection(pika.ConnectionParameters(
        host='18.111.42.239', credentials = credentials))
channel = connection.channel()
channel.queue_declare(queue='6.857', durable=True)

def callback(ch, method, properties, body):
    print " [x] Received %r" % (body,)
    rid, mac, dis = body.split('|')
    rid = int(rid)
    dis = float(dis)
    update(rid, mac, dis)
    ui_update()

channel.basic_consume(callback,
                      queue="6.857",
                      no_ack=True)
channel.start_consuming()
