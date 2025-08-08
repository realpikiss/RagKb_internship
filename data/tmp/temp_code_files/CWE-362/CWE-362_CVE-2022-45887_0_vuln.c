static void ttusb_dec_exit_dvb(struct ttusb_dec *dec)
{
	dprintk("%s\n", __func__);

	dvb_net_release(&dec->dvb_net);
	dec->demux.dmx.close(&dec->demux.dmx);
	dec->demux.dmx.remove_frontend(&dec->demux.dmx, &dec->frontend);
	dvb_dmxdev_release(&dec->dmxdev);
	dvb_dmx_release(&dec->demux);
	if (dec->fe) {
		dvb_unregister_frontend(dec->fe);
		if (dec->fe->ops.release)
			dec->fe->ops.release(dec->fe);
	}
	dvb_unregister_adapter(&dec->adapter);
}