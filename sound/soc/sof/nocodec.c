// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
//
// This file is provided under a dual BSD/GPLv2 license.  When using or
// redistributing this file, you may do so under either license.
//
// Copyright(c) 2018 Intel Corporation
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//

#include <linux/module.h>
#include <sound/sof.h>
#include "sof-audio.h"
#include "sof-priv.h"

static struct snd_soc_card sof_nocodec_card = {
	.name = "nocodec", /* the sof- prefix is added by the core */
	.topology_shortname = "sof-nocodec",
	.owner = THIS_MODULE
};

static int sof_nocodec_bes_setup(struct device *dev,
				 struct snd_soc_dai_driver *drv,
				 struct snd_soc_dai_link *links,
				 int link_num, struct snd_soc_card *card)
{
	struct snd_soc_dai_link_component *dlc;
	int i;

	dev_info(dev, "DEBUG: Entered %s\n", __func__);

	if (!drv || !links || !card) {
		dev_err(dev, "ERROR: Invalid input: drv=%p links=%p card=%p\n", drv, links, card);
		return -EINVAL;
	}

	for (i = 0; i < link_num; i++) {
		dev_info(dev, "DEBUG: Configuring dai_link[%d]\n", i);

		dlc = devm_kcalloc(dev, 2, sizeof(*dlc), GFP_KERNEL);
		if (!dlc) {
			dev_err(dev, "ERROR: Failed to allocate dai_link_component\n");
			return -ENOMEM;
		}

		links[i].name = devm_kasprintf(dev, GFP_KERNEL, "NoCodec-%d", i);
		if (!links[i].name) {
			dev_err(dev, "ERROR: Failed to allocate stream name\n");
			return -ENOMEM;
		}

		links[i].stream_name = links[i].name;
		links[i].cpus = &dlc[0];
		links[i].codecs = &snd_soc_dummy_dlc;
		links[i].platforms = &dlc[1];
		links[i].num_cpus = 1;
		links[i].num_codecs = 1;
		links[i].num_platforms = 1;
		links[i].id = i;
		links[i].no_pcm = 1;
		links[i].cpus->dai_name = drv[i].name;

		if (!dev->of_node)
			links[i].platforms->of_node = dev->of_node;
		else
			links[i].platforms->name = dev_name(dev->parent);

		links[i].be_hw_params_fixup = sof_pcm_dai_link_fixup;

		dev_info(dev, "DEBUG: dai_link[%d]: dai_name=%s, platform of_node=%s\n",
				 i, drv[i].name, dev->of_node ? of_node_full_name(dev->of_node) : "NULL");

		if (drv[i].playback.channels_min)
			links[i].dpcm_playback = 1;
		if (drv[i].capture.channels_min)
			links[i].dpcm_capture = 1;
	}

	card->dai_link = links;
	card->num_links = link_num;

	dev_info(dev, "DEBUG: Configured %d dai_links\n", link_num);

	return 0;
}

static int sof_nocodec_setup(struct device *dev,
			     u32 num_dai_drivers,
			     struct snd_soc_dai_driver *dai_drivers)
{
	struct snd_soc_dai_link *links;

	dev_info(dev, "DEBUG: Entered %s\n", __func__);
	dev_info(dev, "DEBUG: num_dai_drivers = %u\n", num_dai_drivers);

	if (!dai_drivers) {
		dev_err(dev, "ERROR: dai_drivers is NULL\n");
		return -EINVAL;
	}

	links = devm_kcalloc(dev, num_dai_drivers,
			     sizeof(struct snd_soc_dai_link), GFP_KERNEL);
	if (!links) {
		dev_err(dev, "ERROR: Failed to allocate dai_link array\n");
		return -ENOMEM;
	}

	return sof_nocodec_bes_setup(dev, dai_drivers, links, num_dai_drivers, &sof_nocodec_card);
}

static int sof_nocodec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct snd_soc_card *card = &sof_nocodec_card;
	struct snd_soc_acpi_mach *mach;
	int ret;

	dev_info(dev, "DEBUG: Entered %s\n", __func__);

	card->dev = dev;
	card->topology_shortname_created = true;

	mach = dev->platform_data;

	dev_info(dev, "DEBUG: mach pointer = %p\n", mach);
	if (!mach) {
		dev_err(dev, "ERROR: mach (platform_data) is NULL\n");
		return -EINVAL;
	}

	dev_info(dev, "DEBUG: mach->mach_params.num_dai_drivers = %u\n", mach->mach_params.num_dai_drivers);
	dev_info(dev, "DEBUG: mach->mach_params.dai_drivers = %p\n", mach->mach_params.dai_drivers);

	if (!mach->mach_params.dai_drivers || !mach->mach_params.num_dai_drivers) {
		dev_err(dev, "ERROR: Invalid mach_params DAI config\n");
		return -EINVAL;
	}

	ret = sof_nocodec_setup(dev,
				 mach->mach_params.num_dai_drivers,
				 mach->mach_params.dai_drivers);
	if (ret < 0) {
		dev_err(dev, "ERROR: sof_nocodec_setup failed with error %d\n", ret);
		return ret;
	}

	ret = devm_snd_soc_register_card(dev, card);
	if (ret < 0)
		dev_err(dev, "ERROR: snd_soc_register_card failed with error %d\n", ret);

	return ret;
}

static struct platform_driver sof_nocodec_audio = {
	.probe = sof_nocodec_probe,
	.driver = {
		.name = "sof-nocodec",
		.pm = &snd_soc_pm_ops,
	},
};
module_platform_driver(sof_nocodec_audio)

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("ASoC SOF NoCodec");
MODULE_AUTHOR("Liam Girdwood");
MODULE_ALIAS("platform:sof-nocodec");
